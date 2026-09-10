#include "app_health.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_iwdg.h"
#include "rtc.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

#define HEARTBEAT_TIMEOUT_MS 5000
#define RTC_BKP_MAGIC   0xABCDU
#define RTC_BKP_MAGIC_REG   RTC_BKP_DR0
#define RTC_BKP_COUNT_REG   RTC_BKP_DR1
#define RTC_BKP_REASON_REG  RTC_BKP_DR2

static volatile uint32_t s_beat[HEART_NUM];
static uint32_t s_reset_count = 0;
static uint32_t s_last_reason = 0;

static uint32_t bkp_read(uint32_t reg)
{
    return HAL_RTCEx_BKUPRead(&hrtc,reg);
}

static void bkp_write(uint32_t reg,uint32_t val)
{
    HAL_RTCEx_BKUPWrite(&hrtc,reg,val);
}

static const char *reason_str(uint32_t code)
{
    switch(code)
    {
        case CRASH_HARDFAULT : return "HARDFAULT";
        case CRASH_TASK_BASE + HEART_INPUT : return "INPUT STUCK";
        case CRASH_TASK_BASE + HEART_SYS : return "SYS STUCK";
        case CRASH_TASK_BASE + HEART_UI : return "UI STUCK";
        default : return "UNKNOWN";
    }
}

void APP_Health_Init(void)
{
    for(uint8_t i = 0; i < HEART_NUM; i++)  s_beat[i] = 0;
    /*首次上电初始化backup域，复位后保留(VBAT掉电才丢失)*/
    if(bkp_read(RTC_BKP_MAGIC_REG) != RTC_BKP_MAGIC)
    {
        bkp_write(RTC_BKP_MAGIC_REG,RTC_BKP_MAGIC);
        bkp_write(RTC_BKP_COUNT_REG,0);
        bkp_write(RTC_BKP_REASON_REG,0);
    }
    /*复位后 magic 仍匹配，需在 if 外读取才能拿到上次崩溃原因与复位次数*/
    s_reset_count = bkp_read(RTC_BKP_COUNT_REG);
    s_last_reason = bkp_read(RTC_BKP_REASON_REG);
}

void APP_Health_Beat(heart_task_t t)
{
    if(t < HEART_NUM)
    s_beat[t] = xTaskGetTickCount();
}

uint32_t APP_Health_GetResetCount(void)
{
    return s_reset_count;
}

const char *APP_Health_GetLastReason(void)
{
    return reason_str(s_last_reason);
}

void APP_Health_CrashDump(uint32_t reason_code)
{
    const char *rs = reason_str(reason_code);
    bkp_write(RTC_BKP_COUNT_REG,s_reset_count + 1);
    bkp_write(RTC_BKP_REASON_REG,reason_code);
    /*best-effort写CRASH.LOG,失败不阻塞复位*/
    FIL file;
    UINT bw = 0;
    if(f_open(&file,"CRASH.LOG",FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
    {
        f_write(&file,rs,(UINT)strlen(rs),&bw);
        f_close(&file);
    }
    printf("[CRASH] %s\r\n",rs);
    fflush(stdout);
    NVIC_SystemReset();
    for(;;){}
}

/*清空复位历史：RTC 备份域计数/原因与内存缓存同时归零，并删除 CRASH.LOG*/
void APP_Health_ClearHistory(void)
{
    taskENTER_CRITICAL();
    s_reset_count = 0;
    s_last_reason = 0;
    bkp_write(RTC_BKP_COUNT_REG,0);
    bkp_write(RTC_BKP_REASON_REG,0);
    taskEXIT_CRITICAL();

    /*SD 未挂载/文件不存在时忽略失败，不影响计数清零*/
    f_unlink("CRASH.LOG");
}

void APP_Health_Task(void *arg)
{
    (void) arg;
    for(;;)
    {
        BSP_IWDG_Feed();
        uint32_t now = xTaskGetTickCount();
        for(uint8_t i = 0; i < HEART_NUM; i++)
        {
            if(s_beat[i] != 0 && (now - s_beat[i]) > pdMS_TO_TICKS(HEARTBEAT_TIMEOUT_MS))
            {
                APP_Health_CrashDump(CRASH_TASK_BASE + i);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
