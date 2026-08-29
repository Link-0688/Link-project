#include "app_log.h"
#include "app_ui_model.h"
#include "dev_spi_oled.h"
#include "bsp_key.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

#define LOG_ENTRY_MAX   32
#define LOG_MSG_LEN     32
#define LOG_VISIBLE_ROWS  3   /*日志可见行数(y=16/32/48)*/

static char s_log[LOG_ENTRY_MAX][LOG_MSG_LEN];
static uint16_t s_log_head; /*下一条写入位置*/
static uint16_t s_log_count;
static uint8_t  s_scroll;   /*查看时的滚动偏移*/
static uint8_t  s_clear_confirm;    /*0 = 正常 1 = 清空确认*/

void APP_Log_Init(void)
{
    s_log_head = 0;
    s_log_count = 0;
    s_scroll = 0;
    s_clear_confirm = 0;
}

void APP_Log_Exit(void)
{
    /*无需代码*/
}

/*进入日志查看界面：只复位滚动位置，不清空已累计的日志*/
void APP_Log_Open(void)
{
    s_scroll = 0;
    s_clear_confirm = 0;
}

/*记录日志(自带时间戳 mm:ss)*/
void APP_Log_Add(const char *msg)
{
    uint32_t sec = (uint32_t)(xTaskGetTickCount() / 1000);
    char line[LOG_MSG_LEN];
    snprintf(line,LOG_MSG_LEN,"%02u : %02u %s",(unsigned)(sec / 60),(unsigned)(sec % 60),msg);

    APP_UIModel_Lock();
    strncpy(s_log[s_log_head],line,LOG_MSG_LEN - 1);
    s_log[s_log_head][LOG_MSG_LEN - 1] = '\0';
    s_log_head = (uint16_t)((s_log_head + 1) % LOG_ENTRY_MAX);
    if(s_log_count < LOG_ENTRY_MAX)
    {
        s_log_count++;
    }
    APP_UIModel_Unlock();
}

void APP_Log_Clear(void)
{
    s_log_head = 0;
    s_log_count = 0;
    s_scroll = 0;
}

void APP_Log_Dump(void)
{
    if(s_log_count == 0)
    {
        printf("(no log)\r\n");
        return;
    }
    for(uint16_t i = 0; i < s_log_count; i++)
    {
        uint16_t idx = (uint16_t)((s_log_head + LOG_ENTRY_MAX - 1 - i) % LOG_ENTRY_MAX);
        printf("%s\r\n",s_log[idx]);
    }
}

void APP_Log_HandleEvent(input_event_t *p_evt)
{
    APP_UIModel_Lock();
    if(s_clear_confirm)
    {
        if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_1)
        {
            APP_Log_Clear();
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

    if(p_evt->type == EVT_ENC_RIGHT)
    {
        if(s_scroll > 0)    s_scroll--;
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_ENC_LEFT)
    {
        if(s_scroll < s_log_count - 1)  s_scroll++;
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_4)
    {
        if(s_log_count > 0)
        {
            s_clear_confirm = 1;
            g_ui_model.dirty = 1;
        }
    }
    APP_UIModel_Unlock();
}

void APP_Log_Render(void)
{
    if(s_clear_confirm)
    {
        dev_oled_show_string(0,0,"CLEAR LOG?");
        dev_oled_show_string(0,24,"1 = YES");
        dev_oled_show_string(0,40,"2 = NO");
        return;
    }

    char title[16];
    snprintf(title,sizeof(title),"LOG %u/%u",
             (unsigned)(s_scroll + 1),(unsigned)s_log_count);
    dev_oled_show_string(0,0,title);

    if(s_log_count == 0)
    {
        dev_oled_show_string(16,28,"NO LOG");
        return;
    }

    for(uint8_t i = 0; i < LOG_VISIBLE_ROWS; i++)
    {
        uint16_t idx = (uint16_t)((s_log_head + LOG_ENTRY_MAX - 1 - s_scroll - i) % LOG_ENTRY_MAX);
        if(i + s_scroll >= s_log_count)     break;
        dev_oled_show_string(0,(uint16_t)(16 + i * 16),s_log[idx]);
    }
}
