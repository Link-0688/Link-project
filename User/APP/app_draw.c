#include "app_draw.h"
#include "app_ui_model.h"
#include "app_sys.h"
#include "dev_spi_oled.h"
#include "dev_storage.h"
#include "dev_config.h"
#include "bsp_key.h"
#include "ff.h"
#include <string.h>

#define CANVAS_W    128 //宽
#define CANVAS_H    64  //高
#define CANVAS_BYTES    (8 * 128)   /*1024Bytes*/

static uint8_t g_draw_canvas[8][128];   /*独立画布，不直接用显存*/
static uint8_t s_cursor_x;
static uint8_t s_cursor_y;
static uint8_t s_clear_confirm; /*0 = 正常 1 = 清空确认*/

/*画布像素操作*/
static void canvas_set_pixel(uint8_t x,uint8_t y)
{
    if(x >= CANVAS_W || y >= CANVAS_H)  return;//防止坐标越界引发内存写穿
    g_draw_canvas[y >> 3][x] |= (uint8_t)(1 << (y & 7));//按位或|=点亮
}

static void canvas_clear_pixel(uint8_t x,uint8_t y)
{
    if(x >= CANVAS_W || y >= CANVAS_H)  return;
    g_draw_canvas[y >> 3][x] &= (uint8_t)~(1 << (y & 7));
}

/*落盘/加载*/
static void save_canvas(void)
{
    if(!APP_Sys_IsSdReady())    return;
    DEV_Storage_Write("DRAW.PIC", (const uint8_t*)g_draw_canvas, CANVAS_BYTES);
}

static void load_canvas(void)
{
    if(!APP_Sys_IsSdReady())    return;
    if(!DEV_Storage_Read("DRAW.PIC", (uint8_t*)g_draw_canvas, CANVAS_BYTES))  
    {
        memset(g_draw_canvas, 0, CANVAS_BYTES);   /* 损坏/不存在 -> 空画布 */
    }
}

void APP_Draw_Init(void)
{
    memset(g_draw_canvas,0,CANVAS_BYTES);
    s_cursor_x = 64;
    s_cursor_y = 32;
    s_clear_confirm = 0;
    load_canvas();
}

void APP_Draw_Exit(void)
{
    save_canvas();
}

/*落笔(画一个size*size方块)*/
static void draw_brush(void)
{
    config_t cfg = DEV_Config_Get();
    uint8_t size = cfg.cursor_size;
    if(size == 0)   size = 1;
    if(size > 4)    size = 4;

    for(uint8_t dy = 0; dy < size; dy++)
    {
        for(uint8_t dx = 0; dx < size; dx++)
        {
            canvas_set_pixel((uint8_t)(s_cursor_x + dx),(uint8_t)(s_cursor_y + dy));
        }
    }
}

/*擦除*/
static void erase_brush(void)
{
    config_t cfg = DEV_Config_Get();
    uint8_t size = cfg.cursor_size;
    if(size == 0)   size = 1;
    if(size > 4)    size = 4;
    for(uint8_t dy = 0; dy < size; dy++)
    {
        for(uint8_t dx = 0; dx < size; dx++)
        {
            canvas_clear_pixel((uint8_t)(s_cursor_x + dx),(uint8_t)(s_cursor_y + dy));
        }
    }
}

void APP_Draw_HandleEvent(input_event_t *p_evt)
{
    config_t cfg = DEV_Config_Get();
    uint8_t sens = cfg.sensitivity;
    if(sens == 0) sens = 1;

    APP_UIModel_Lock();
    /*清空确认*/
    if(s_clear_confirm)
    {
        if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_1)
        {
            memset(g_draw_canvas,0,CANVAS_BYTES);
            save_canvas();
            s_clear_confirm = 0;
            g_ui_model.dirty = 1;
        }
        else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_2)
        {
            s_clear_confirm = 0;
            g_ui_model.dirty = 1;
        }
        APP_UIModel_Unlock();
        return;
    }

    /*长按KEY_3进入清空确认*/
    if(p_evt->type == EVT_KEY_LONG && p_evt->param ==KEY_3)
    {
        s_clear_confirm = 1;
        g_ui_model.dirty = 1;
        APP_UIModel_Unlock();
        return;
    }

    if(p_evt->type == EVT_ENC_RIGHT)
    {
        int16_t step = (int16_t)(p_evt->param * sens);
        if(s_cursor_x + step < CANVAS_W)    s_cursor_x = (uint8_t)(s_cursor_x + step);
        else s_cursor_x = CANVAS_W - 1;
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_ENC_LEFT)
    {
        int16_t step = (int16_t)(p_evt->param * sens);
        if(s_cursor_x >= step)  s_cursor_x = (uint8_t)(s_cursor_x - step);
        else s_cursor_x = 0;
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_1)
    {
        if(s_cursor_y > 0)  s_cursor_y--;
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_2)
    {
        if(s_cursor_y < CANVAS_H - 1)   s_cursor_y++;
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_3)
    {
        draw_brush();   /*落笔*/
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_4)
    {
        erase_brush();  /*擦除*/
        g_ui_model.dirty = 1;
    }
    APP_UIModel_Unlock();
}

void APP_Draw_Render(void)
{
    /*1.画布拷贝到显存*/
    memcpy(g_u8_oled_gram,g_draw_canvas,CANVAS_BYTES);
    /*2.画光标框(边界 clamp，避免 uint8_t 下溢)*/
    uint8_t x = s_cursor_x;
    uint8_t y = s_cursor_y;
    int16_t x0 = (int16_t)x - 1; if(x0 < 0) x0 = 0;
    int16_t y0 = (int16_t)y - 1; if(y0 < 0) y0 = 0;
    uint8_t x1 = (x + 1 < CANVAS_W) ? (uint8_t)(x + 1) : (uint8_t)(CANVAS_W - 1);
    uint8_t y1 = (y + 1 < CANVAS_H) ? (uint8_t)(y + 1) : (uint8_t)(CANVAS_H - 1);
    dev_oled_draw_rectangle((uint8_t)x0, (uint8_t)y0, x1, y1, 0);
    /*3.界面清除*/
    {
        if(s_clear_confirm)
        {
            dev_oled_show_string(0,0,"CLEAR ALL?");
            dev_oled_show_string(0,24,"1 = YES");
            dev_oled_show_string(0,40,"2 = NO");
            return;
        }
    }
}
