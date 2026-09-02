#include "app_setting.h"
#include "app_ui_model.h"
#include "dev_spi_oled.h"
#include "dev_config.h"
#include "bsp_buzzer.h"
#include "bsp_key.h"
#include <stdio.h>

#define SET_VISIBLE_ROWS    4   /*一屏最多显示 4 行(64/16)*/

typedef enum{
    SET_SENSITIVITY = 0,
    SET_CURSOR_SIZE,
    SET_BRIGHTNESS,
    SET_VOLUME,
    SET_SLEEP_TIME,
    SET_NUM
}setting_item_t;

static config_t s_cfg;  /*编辑中的临时配置*/
static uint8_t  s_item; /*选中项*/
static uint8_t  s_edit; /*0 = 浏览 1 = 编辑*/
static uint8_t  s_scroll;

/*把亮度（0~100）映射到对比度（0~255）并立即生效*/
static void apply_brightness(void)
{
    uint8_t contrast = (uint8_t)(s_cfg.brightness * 255 / 100);
    dev_oled_set_contrast(contrast);
}

/*让滚动窗口跟随选中项，处理回绕：选中项滚出顶部或底部时整屏跟随*/
static void scroll_to_item(void)
{
    if(s_item < s_scroll)
    {
        s_scroll = s_item;
    }
    else if(s_item >= (uint8_t)(s_scroll + SET_VISIBLE_ROWS))
    {
        s_scroll = (uint8_t)(s_item - SET_VISIBLE_ROWS + 1);
    }
}

void APP_Setting_Init(void)
{
    s_cfg = DEV_Config_Get();
    s_item = 0;
    s_edit = 0;
    s_scroll = 0;
}

void APP_Setting_Exit(void)
{
    DEV_Config_Set(&s_cfg);
    DEV_Config_Save();
    apply_brightness();
    BSP_Buzzer_SetVolume(s_cfg.volume);
}

void APP_Setting_HandleEvent(input_event_t *p_evt)
{
    APP_UIModel_Lock();
    if(!s_edit)
    {
        /*浏览模式：选设置项*/
        if(p_evt->type == EVT_ENC_RIGHT)
        {
            s_item = (uint8_t)((s_item + 1) % SET_NUM);
            scroll_to_item();
            g_ui_model.dirty = 1;
        }
        else if(p_evt->type == EVT_ENC_LEFT)
        {
            s_item = (uint8_t)((s_item + SET_NUM - 1) % SET_NUM);
            scroll_to_item();
            g_ui_model.dirty = 1;
        }
        else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_3)
        {
            s_edit = 1;/*进入编辑*/
            g_ui_model.dirty = 1;
        }
    }
    else
    {
        /*编辑模式：调值*/
        int16_t delta = 0;
        if(p_evt->type == EVT_ENC_RIGHT) delta = 1;
        else if(p_evt->type == EVT_ENC_LEFT) delta = -1;

        switch(s_item){
            case SET_SENSITIVITY:
                if(s_cfg.sensitivity + delta >= 1 && s_cfg.sensitivity + delta <= 10)
                {
                    s_cfg.sensitivity = (uint8_t)(s_cfg.sensitivity + delta);
                }
                break;
            case SET_CURSOR_SIZE:
                if(s_cfg.cursor_size + delta >= 1 && s_cfg.cursor_size + delta <= 4)
                {
                    s_cfg.cursor_size = (uint8_t)(s_cfg.cursor_size + delta);
                }
                break;
            case SET_BRIGHTNESS:
                if(s_cfg.brightness + delta >= 0 && s_cfg.brightness + delta <= 100)
                {
                    s_cfg.brightness = (uint8_t)(s_cfg.brightness + delta);
                    apply_brightness();
                }
                break;
            case SET_VOLUME:
                if(s_cfg.volume + delta >= 0 &&s_cfg.volume + delta <= 100)
                    s_cfg.volume = (uint8_t)(s_cfg.volume + delta);
                break;
            case SET_SLEEP_TIME:
                if(s_cfg.sleep_time + delta >= 5 && s_cfg.sleep_time + delta <= 120)
                {
                    s_cfg.sleep_time = (uint16_t)(s_cfg.sleep_time + delta);
                }
                break;
        }
        if(p_evt->type == EVT_ENC_RIGHT || p_evt->type == EVT_ENC_LEFT)
            g_ui_model.dirty = 1;

        if(p_evt->type == EVT_KEY_PRESS && p_evt->param ==KEY_3)
        {
            s_edit = 0; /*确认，回浏览*/
            g_ui_model.dirty = 1;
        }
        else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_4)
        {
            s_edit = 0; /*取消(已改的直接保留)*/
            g_ui_model.dirty = 1;
        }
    }
    APP_UIModel_Unlock();
}

void APP_Setting_Render(void)
{
    for(uint8_t i = 0; i < SET_NUM; i++)
    {
        int16_t row = (int16_t)(i - s_scroll);   /*相对滚动窗口的行号*/
        if(row < 0 || row >= SET_VISIBLE_ROWS)   /*跳过屏幕外的项*/
        {
            continue;
        }
        uint8_t y = (uint8_t)(row * 16);
        char buf[24];
        if(i == s_item)
        {
            dev_oled_show_string(0,y,">");
        }
        switch(i)
        {
            case SET_SENSITIVITY : snprintf(buf,sizeof(buf),"SENS   %u",s_cfg.sensitivity); break;
            case SET_CURSOR_SIZE : snprintf(buf,sizeof(buf),"SIZE   %u",s_cfg.cursor_size); break;
            case SET_BRIGHTNESS : snprintf(buf,sizeof(buf),"BRIGHT %u%%",s_cfg.brightness); break;
            case SET_VOLUME      : snprintf(buf, sizeof(buf), "VOL    %u%%", s_cfg.volume); break;
            case SET_SLEEP_TIME : snprintf(buf,sizeof(buf),"SLEEP   %us",s_cfg.sleep_time); break;
        }
        dev_oled_show_string(8,y,buf);
    }
}
