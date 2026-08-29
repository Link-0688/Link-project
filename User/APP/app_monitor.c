#include "app_monitor.h"
#include "app_ui_model.h"
#include "dev_spi_oled.h"
#include "bsp_key.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define TASK_LIST_BUF       512
#define MON_TASK_VISIBLE    3   /*任务列表区可见行数(第0行是标题, 任务从 y=16 起)*/
#define MON_CPU_TASK_MAX    16  /*最多统计的任务数*/
static char s_task_buf[TASK_LIST_BUF];
static uint8_t s_scroll;
static uint8_t s_max_scroll;    /*最大滚动值 = 任务总行数 - 可见行数, 由 Render 计算*/
static uint8_t s_page;  /*0 = 任务列表，1 = CPU占用*/

void APP_Monitor_Init(void)
{
    s_scroll = 0;
    s_max_scroll = 0;
    s_page = 0;
}

void APP_Monitor_Exit(void)
{

}

void APP_Monitor_HandleEvent(input_event_t *p_evt)
{
    APP_UIModel_Lock();
    if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_3)
    {
        /*KEY3在任务列表页和CPU占用页之间切换*/
        s_page = (uint8_t)(1 - s_page);
        s_scroll = 0;
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_ENC_LEFT && s_scroll < s_max_scroll)
    {
        s_scroll++;
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_ENC_RIGHT && s_scroll > 0)
    {
        s_scroll--;
        g_ui_model.dirty = 1;
    }
    APP_UIModel_Unlock();
}

/*页0：任务列表(vTaskList)*/
static void render_task_list(void)
{
    uint32_t sec = (uint32_t)(xTaskGetTickCount() / 1000);
    char line[24];
    snprintf(line,sizeof(line),"UP:%lus Task:%lu",(unsigned long)sec,(unsigned long)uxTaskGetNumberOfTasks());
    dev_oled_show_string(0,0,line);
    /*生成任务列表*/
    vTaskList(s_task_buf);
    /*统计任务总行数，防止滚过头只剩空白行*/
    uint8_t total = 0;
    for(char *p = s_task_buf; *p; p++)
    {
        if(*p == '\n')  total++;
    }
    s_max_scroll = (total > MON_TASK_VISIBLE) ? (uint8_t)(total - MON_TASK_VISIBLE) : 0;
    if(s_scroll > s_max_scroll) s_scroll = s_max_scroll;
    /*按行显示：每行取任务名 + 堆栈高水位*/
    char *p_row = s_task_buf;
    uint8_t row_idx = 0;
    while(*p_row)
    {
        char *p_end = strchr(p_row,'\n');
        if(p_end == NULL)   break;
        *p_end = '\0';
        char *p_tab1 = strchr(p_row,'\t');
        if(p_tab1 != NULL) *p_tab1 = '\0';
        if(row_idx >= s_scroll)
        {
            uint8_t disp = (uint8_t)(row_idx - s_scroll);
            if(disp >= MON_TASK_VISIBLE)    break;  /*越界保护：只画可见区*/
            uint8_t y = (uint8_t)(16 + disp * 16);
            char out[48];
            snprintf(out,sizeof(out),"%.15s %s",p_row,(p_tab1 != NULL) ? (p_tab1 + 1) : " ");
            dev_oled_show_string(8,y,out);
        }
        row_idx++;
        p_row = p_end + 1;
    }
}

/*页1：CPU占用率 + 各任务运行时间占比*/
static void render_cpu(void)
{
    TaskStatus_t tasks[MON_CPU_TASK_MAX];
    uint32_t ul_total;
    UBaseType_t n = uxTaskGetSystemState(tasks,MON_CPU_TASK_MAX,&ul_total);
    /*缩放：除以100，防止100*ulRunTimeCounter乘法溢出，同时得到百分比分母*/
    uint32_t ul_scaled = ul_total / 100;
    /*空闲任务占比反推总占用率*/
    uint8_t idle_pct = 0;
    for(UBaseType_t i = 0; i < n; i++)
    {
        if(strcmp(tasks[i].pcTaskName,"IDLE") == 0)
        {
            if(ul_scaled > 0)
            idle_pct = (uint8_t)(tasks[i].ulRunTimeCounter / ul_scaled);
            break;
        }
    }
    uint8_t cpu_pct = (uint8_t)(100 - idle_pct);
    char line[24];
    snprintf(line,sizeof(line),"CPU:%u%%",cpu_pct);
    dev_oled_show_string(0,0,line);
    /*任务行：从y = 16起，可见MON_TASK_VISIBLE行*/
    s_max_scroll = (n > MON_TASK_VISIBLE) ? (uint8_t)(n - MON_TASK_VISIBLE) : 0;
    if(s_scroll > s_max_scroll) s_scroll = s_max_scroll;
    for(UBaseType_t i = 0; i < n; i++)
    {
        if(i < s_scroll)    continue;
        uint8_t disp = (uint8_t)(i - s_scroll);
        if(disp >= MON_TASK_VISIBLE)    break;
        uint8_t y = (uint8_t)(16 + disp * 16);
        uint8_t pct = 0;
        if(ul_scaled > 0)
        pct = (uint8_t)(tasks[i].ulRunTimeCounter / ul_scaled);
        char row[24];
        snprintf(row,sizeof(row),"%.10s %u%%",tasks[i].pcTaskName,pct);
        dev_oled_show_string(0,y,row);
    }
}

void APP_Monitor_Render(void)
{
    if(s_page == 0) render_task_list();
    else render_cpu();
}
