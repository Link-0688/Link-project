#ifndef __APP_HEALTH_H
#define __APP_HEALTH_H

#include <stdint.h>

/*崩溃原因编码*/
#define CRASH_HARDFAULT 1
#define CRASH_TASK_BASE 2   /*HEART_INPUT/SYS/UI*/

typedef enum{
    HEART_INPUT = 0,
    HEART_SYS,
    HEART_UI,
    HEART_NUM
}heart_task_t;

void APP_Health_Init(void);
void APP_Health_Beat(heart_task_t t);   /*任务循环里调用，更新心跳*/
void APP_Health_Task(void *arg);        /*TaskHealth 入口*/
void APP_Health_CrashDump(uint32_t reason_code);    /*崩溃转储+复位*/

/*供监控页读取上次异常摘要*/
uint32_t APP_Health_GetResetCount(void);
const char *APP_Health_GetLastReason(void);

#endif /*__APP_HEALTH_H*/
