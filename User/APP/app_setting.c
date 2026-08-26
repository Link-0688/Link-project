#include "app_setting.h"
#include "app_ui_model.h"
#include "dev_spi_oled.h"
#include "dev_config.h"
#include "bsp_key.h"
#include <stdio.h>

typedef enum{
    SET_SENSITIVITY = 0,
    SET_CURSOR_SIZE,
    SET_BRIGHTNESS,
    SET_SLEEP_TIME,
    SET_NUM
}setting_item_t;

static config_t s_cfg;  /*编辑中的临时配置*/
static uint8_t  s_item; /*选中项*/
static uint8_t  s_edit; /*0 = 浏览 1 = 编辑*/

/*把亮度（0~100）映射到对比度（0~255）并立即生效*/
static void apply_brightness(void)
{
    uint8_t contrast = (uint8_t)(s_cfg.brightness * 255 / 100);
    dev_oled_set_contrast(contrast);
}

void APP_Setting_Init(void)
{
    s_cfg = DEV_Config_Get();
    s_item = 0;
    s_edit = 0;
}

void APP_Setting_Exit(void)
{
    DEV_Config_Set(&s_cfg);
    DEV_Config_Save();
    apply_brightness();
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
        }
        else if(p_evt->type == EVT_ENC_LEFT)
        {
            s_item = (uint8_t)((s_item + SET_NUM - 1) % SET_NUM);
        }
        else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_3)
        {
            s_edit = 1;/*进入编辑*/
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
            case SET_SLEEP_TIME:
                if(s_cfg.sleep_time + delta >= 5 && s_cfg.sleep_time + delta <= 120)
                {
                    s_cfg.sleep_time = (uint16_t)(s_cfg.sleep_time + delta);
                }
                break;
        }
        if(p_evt->type == EVT_KEY_PRESS && p_evt->param ==KEY_3)
        {
            s_edit = 0; /*确认，回浏览*/
        }
        else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_4)
        {
            s_edit = 0; /*取消(已改的直接保留)*/
        }
    }
    APP_UIModel_Unlock();
}

void APP_Setting_Render(void)
{
    for(uint8_t i = 0; i < SET_NUM; i++)
    {
        uint8_t y = (uint8_t)(0 + i * 16);
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
            case SET_SLEEP_TIME : snprintf(buf,sizeof(buf),"SLEEP   %us",s_cfg.sleep_time); break;
        }
        dev_oled_show_string(8,y,buf);
    }
}
