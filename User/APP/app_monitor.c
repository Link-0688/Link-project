#include "app_monitor.h"
#include "app_ui_model.h"
#include "app_health.h"
#include "dev_spi_oled.h"
#include "bsp_key.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define MON_TASK_VISIBLE    3   /*任务列表区可见行数(第0行是标题, 任务从 y=16 起)*/
#define MON_CPU_TASK_MAX    16  /*最多统计的任务数*/
static uint8_t s_scroll;
static uint8_t s_max_scroll;    /*最大滚动值 = 任务总行数 - 可见行数, 由 Render 计算*/
static uint8_t s_page;  /*0 = 任务列表，1 = CPU占用 2 = 健康摘要*/

/* 任务状态 -> 单字符(vTaskList 同款) */
static char task_state_char(eTaskState st)
{
    switch(st)
    {
        case eRunning:   return 'X';
        case eReady:     return 'R';
        case eBlocked:   return 'B';
        case eSuspended: return 'S';
        case eDeleted:   return 'D';
        default:         return '?';
    }
}

/* 按优先级降序排序：任务优先级固定，避免 Ready/Blocked 切换导致列表位置跳变 */
static void sort_tasks_by_priority(TaskStatus_t *tasks, UBaseType_t n)
{
    for(UBaseType_t i = 0; i + 1 < n; i++)
    {
        for(UBaseType_t j = i + 1; j < n; j++)
        {
            if(tasks[j].uxCurrentPriority > tasks[i].uxCurrentPriority)
            {
                TaskStatus_t tmp = tasks[i];
                tasks[i] = tasks[j];
                tasks[j] = tmp;
            }
        }
    }
}

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
        /*KEY3在任务列表/CPU占用/健康摘要三页间循环切换*/
        s_page = (uint8_t)((s_page + 1) % 3);
        s_scroll = 0;
        g_ui_model.dirty = 1;
    }
    else if(s_page != 2 && p_evt->type == EVT_ENC_LEFT && s_scroll < s_max_scroll)
    {
        s_scroll++;
        g_ui_model.dirty = 1;
    }
    else if(s_page != 2 && p_evt->type == EVT_ENC_RIGHT && s_scroll > 0)
    {
        s_scroll--;
        g_ui_model.dirty = 1;
    }
    APP_UIModel_Unlock();
}

/*页0：任务列表(名称 + 状态 + 堆栈高水位)*/
static void render_task_list(void)
{
    uint32_t sec = (uint32_t)(xTaskGetTickCount() / 1000);
    char line[24];
    snprintf(line,sizeof(line),"UP:%lus Task:%lu",(unsigned long)sec,(unsigned long)uxTaskGetNumberOfTasks());
    dev_oled_show_string(0,0,line);

    TaskStatus_t tasks[MON_CPU_TASK_MAX];
    uint32_t ul_total;
    UBaseType_t n = uxTaskGetSystemState(tasks,MON_CPU_TASK_MAX,&ul_total);
    sort_tasks_by_priority(tasks,n);

    s_max_scroll = (n > MON_TASK_VISIBLE) ? (uint8_t)(n - MON_TASK_VISIBLE) : 0;
    if(s_scroll > s_max_scroll) s_scroll = s_max_scroll;

    for(UBaseType_t i = 0; i < n; i++)
    {
        if(i < s_scroll)    continue;
        uint8_t disp = (uint8_t)(i - s_scroll);
        if(disp >= MON_TASK_VISIBLE)    break;
        uint8_t y = (uint8_t)(16 + disp * 16);
        char out[48];
        snprintf(out,sizeof(out),"%.12s %c %u",tasks[i].pcTaskName,
                 task_state_char(tasks[i].eCurrentState),
                 (unsigned)tasks[i].usStackHighWaterMark);
        dev_oled_show_string(0,y,out);
    }
}

/*页1：CPU占用率 + 各任务运行时间占比*/
static void render_cpu(void)
{
    TaskStatus_t tasks[MON_CPU_TASK_MAX];
    uint32_t ul_total;
    UBaseType_t n = uxTaskGetSystemState(tasks,MON_CPU_TASK_MAX,&ul_total);
    sort_tasks_by_priority(tasks,n);
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

/* 页2: 健康摘要 (上次异常原因 + 异常复位计数) */
static void render_health(void)
{
    dev_oled_show_string(0, 0, "HEALTH");
    char line[24];
    snprintf(line, sizeof(line), "RST:%lu", (unsigned long)APP_Health_GetResetCount());
    dev_oled_show_string(0, 16, line);

    char reason[24];
    snprintf(reason, sizeof(reason), "%s", APP_Health_GetLastReason());
    dev_oled_show_string(0, 32, reason);

    dev_oled_show_string(0, 48, "KEY3 NEXT");
}

void APP_Monitor_Render(void)
{
    if(s_page == 0) render_task_list();
    else if(s_page == 1) render_cpu();
    else render_health();
}
