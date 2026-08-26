#include "app_monitor.h"
#include "app_ui_model.h"
#include "dev_spi_oled.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define TASK_LIST_BUF       512
#define MON_TASK_VISIBLE    3   /*任务列表区可见行数(第0行是标题, 任务从 y=16 起)*/
static char s_task_buf[TASK_LIST_BUF];
static uint8_t s_scroll;
static uint8_t s_max_scroll;    /*最大滚动值 = 任务总行数 - 可见行数, 由 Render 计算*/

void APP_Monitor_Init(void)
{
    s_scroll = 0;
    s_max_scroll = 0;
}

void APP_Monitor_Exit(void)
{

}

void APP_Monitor_HandleEvent(input_event_t *p_evt)
{
    APP_UIModel_Lock();
    if(p_evt->type == EVT_ENC_RIGHT && s_scroll > 0)
    {
        s_scroll--;
    }
    else if (p_evt->type == EVT_ENC_LEFT && s_scroll < s_max_scroll)
    {
        s_scroll++;
    }
    APP_UIModel_Unlock();
}

void APP_Monitor_Render(void)
{
    uint32_t sec = (uint32_t)(xTaskGetTickCount() / 1000);
    char line[24];
    snprintf(line,sizeof(line),"UP:%lus Task:%lu",(unsigned long)sec,(unsigned long)uxTaskGetNumberOfTasks());
    dev_oled_show_string(0,0,line);

    /*生成任务列表*/
    vTaskList(s_task_buf);

    /*统计任务总行数，建立防护边界，防止滚过头后只剩空白行*/
    uint8_t total = 0;
    for(char *p = s_task_buf; *p; p++)
    {
        if(*p == '\n')  total++;
    }
    s_max_scroll = (total > MON_TASK_VISIBLE) ? (uint8_t)(total - MON_TASK_VISIBLE) : 0;
    if(s_scroll > s_max_scroll)  s_scroll = s_max_scroll;

    /*按行显示：每行取任务名 + 堆栈高水位*/
    char *p_row = s_task_buf;
    uint8_t row_idx = 0;
    while(*p_row)
    {
        /*找这一行的行尾*/
        char *p_end = strchr(p_row,'\n');
        if(p_end == NULL)   break;
        *p_end = '\0';

        /*拆出第一个字段(任务名)*/
        char *p_tab1 = strchr(p_row,'\t');
        if(p_tab1 != NULL) *p_tab1 = '\0';
        if(row_idx >= s_scroll)
        {
            uint8_t disp = (uint8_t)(row_idx - s_scroll);
            if(disp >= MON_TASK_VISIBLE) break;   /*越界保护：只画可见区*/
            uint8_t y = (uint8_t)(16 + disp * 16);
            char out[48];
            snprintf(out,sizeof(out),"%.15s %s",p_row,(p_tab1 != NULL) ? (p_tab1 + 1) : " ");
            dev_oled_show_string(8,y,out);
        }
        row_idx++;
        p_row = p_end + 1;
    }
}
