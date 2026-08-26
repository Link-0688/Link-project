#include "app_ui_model.h"
#include "dev_spi_oled.h"
#include "rtc.h"
#include <stdio.h>
#include <string.h>

/*第三阶段新增内容*/
#include "app_file.h"
#include "app_draw.h"
#include "app_music.h"
#include "app_log.h"
#include "app_monitor.h"
#include "app_setting.h"

/*桌面图标*/
typedef struct{
    uint8_t     x,y;
    const char *name;
}icon_t;

static const icon_t s_icons[DESKTOP_ICON_NUM] = {
    {4,16,"FILE"},
    {46,16,"DRAW"},
    {88,16,"MUSIC"},
    {4,40,"LOG"},
    {46,40,"MON"},
    {88,40,"SET"},
};

/*读RTC时间到模型*/
static void read_rtc(ui_model_t *p_m)
{
    RTC_TimeTypeDef s_time = {0};
    RTC_DateTypeDef s_date = {0};

    /* 必须成对调用：先 GetTime，再 GetDate */
    HAL_RTC_GetTime(&hrtc, &s_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &s_date, RTC_FORMAT_BIN);

    p_m->hour   = s_time.Hours;
    p_m->minute = s_time.Minutes;
    p_m->second = s_time.Seconds;
}

/*各界面渲染*/
static void render_boot(ui_model_t *p_m)
{
    dev_oled_show_string(0,0,"DEVICE:");
    dev_oled_show_string(56,0,p_m->device_connected ? "NORMAL" : "ERROR!");
    dev_oled_show_string(20,16,"Micro Desktop");
    char buf[16];
    snprintf(buf,sizeof(buf),"%02d:%02d:%02d",p_m->hour,p_m->minute,p_m->second);
    dev_oled_show_string(30,32,buf);
    dev_oled_show_string(0,48,"LONG KEY2->LOGIN");
}

static void render_login(ui_model_t *p_m)
{
    dev_oled_show_string(0,0,"PASSWORD:");
    char buf[16];
    for(uint8_t i = 0; i < 4; i++)
    {
        if(i < p_m->pwd_len)
        {
            buf[i * 2]  = (char)('0' + p_m->pwd_input[i]);
            buf[i * 2 + 1] = ' ';
        }
        else
        {
            buf[i * 2]  = '_';
            buf[i * 2 + 1] = ' ';
        }
    }
    buf[8] = '\0';
    dev_oled_show_string(8,16,buf);
    if(p_m->pwd_error)
    {
        dev_oled_show_string(8,40,"ERROR!");
    }
}

static void render_lock(ui_model_t *p_m)
{
    dev_oled_show_string(30, 24, "LOCKED!");

    char buf[16];
    snprintf(buf,sizeof(buf), "WAIT %ds", p_m->lock_remain_s);
    dev_oled_show_string(36, 40, buf);
}

static void render_desktop(ui_model_t *p_m)
{
    char buf[16];
    snprintf(buf,sizeof(buf),"%02d:%02d:%02d",p_m->hour,p_m->minute,p_m->second);
    dev_oled_show_string(0,0,buf);
    dev_oled_show_string(102,0,p_m->device_connected ? "OK" : "ERR");
    for(uint8_t i = 0; i < DESKTOP_ICON_NUM; i++)
    {
        uint8_t x = s_icons[i].x;
        uint8_t y = s_icons[i].y;
        uint8_t w = (uint8_t)(strlen(s_icons[i].name) * 8);
        if(i == p_m->cursor_index)
        {
            dev_oled_draw_rectangle((uint8_t)(x-2),(uint8_t)(y-2),
                                    (uint8_t)(x+w+1),(uint8_t)(y+17),0);
        }
        dev_oled_show_string(x,y,s_icons[i].name);
    }
}

/*渲染任务入口*/
void TaskUI(void *argument)
{
    (void)argument;
    dev_oled_init();
    dev_oled_clear();
    for(;;)
    {
        APP_UIModel_Lock();
        ui_model_t m = g_ui_model;/*渲染用局部数据*/
        APP_UIModel_Unlock();
        read_rtc(&m);
        if(m.state != SYS_STATE_SCREEN_OFF)
        {
            dev_oled_clear();
            switch(m.state)
            {
                case SYS_STATE_BOOT : render_boot(&m); break;
                case SYS_STATE_LOGIN : render_login(&m); break;
                case SYS_STATE_LOCK : render_lock(&m); break;
                case SYS_STATE_DESKTOP : render_desktop(&m); break;

                case SYS_STATE_APP_FILE : APP_File_Render(); break;
                case SYS_STATE_APP_DRAW : APP_Draw_Render(); break;
                case SYS_STATE_APP_MUSIC : APP_Music_Render(); break;
                case SYS_STATE_APP_LOG : APP_Log_Render(); break;
                case SYS_STATE_APP_MONITOR : APP_Monitor_Render(); break;
                case SYS_STATE_APP_SETTING : APP_Setting_Render(); break;

                default : break;
            }
            dev_oled_refresh_gram();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
