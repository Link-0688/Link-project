#include "app_event.h"
#include "app_input.h"
#include "dev_ec11.h"
#include "bsp_key.h"
#include "timers.h"
#include <stdio.h>

/*检测线(判断输入设备是否连接成功)*/
#define DETECT_PORT     GPIOC
#define DETECT_PIN      GPIO_PIN_4

/*扫描与消抖参数*/
#define ENC_POLL_MS  10  /*扫描周期(任务循环)*/
#define DEBOUNCE_MS     20  /*消抖时间*/
#define LONG_PRESS_MS   2000/*长按时间*/

/*任务通知位：区分中断来源*/
#define NOTIFY_KEY  0x01U   /*KEY1~4+编码器*/
#define NOTIFY_DETECT 0x02U /*检测线*/

static TaskHandle_t s_input_task = NULL; /*TaskInput自身句柄，供ISR通知*/
static TimerHandle_t s_long_timer = NULL; /*长按软件定时器*/

/*消抖后的稳定状态：0 = 松开，1 = 按下*/
static uint8_t s_key_state[KEY_NUM];    /*KEY1~4*/
static uint8_t s_enc_state;             /*编码器键*/
static uint8_t s_detect_state;          /*检测线：0=连接，1=断开*/

/*长按超时回调->发EVT_ENC_LONG*/
static void long_press_cb(TimerHandle_t xTimer)
{
    int16_t param = (int16_t)(intptr_t)pvTimerGetTimerID(xTimer);
    APP_Event_Send(EVT_KEY_LONG,param);
}

/*启动长按定时器*/
static void start_long_press(int16_t param)
{
    if(s_long_timer == NULL)    return;
    vTimerSetTimerID(s_long_timer,(void*)(intptr_t)param);
    xTimerReset(s_long_timer,0);
}

/*停止长按定时器(防止误发LONG)*/
static void stop_long_press(void)
{
    if(s_long_timer == NULL)    return;
    xTimerStop(s_long_timer,0);
}

/*HAL EXTI回调：用任务通知唤醒TaskInput去消抖判边沿*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    switch(GPIO_Pin)
    {
        case GPIO_PIN_5:    /*KEY1*/
        case GPIO_PIN_6:    /*KEY2*/
        case GPIO_PIN_7:    /*KEY3*/
        case GPIO_PIN_8:    /*KEY4*/
        case GPIO_PIN_11:   /*PA11*/
        if(s_input_task != NULL)
        xTaskNotifyFromISR(s_input_task,NOTIFY_KEY,eSetBits,&xHigherPriorityTaskWoken);
        break;
        case GPIO_PIN_4:    /*检测线PC4*/
        if(s_input_task != NULL)
        xTaskNotifyFromISR(s_input_task,NOTIFY_DETECT,eSetBits,&xHigherPriorityTaskWoken);
        break;
        default : break;
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*读按键与编码器键的稳定电平，判断边沿发事件*/
static void scan_keys(void)
{
    for(uint8_t i = 0; i < KEY_NUM; i++)
    {
        uint8_t raw = BSP_Key_Read((key_id_t)i);
        if(raw != s_key_state[i])
        {
            s_key_state[i] = raw;
            if(raw == 1)
            {
                APP_Event_Send(EVT_KEY_PRESS,(int16_t)i);
                start_long_press((int16_t) i);
            }
            else
            {
                APP_Event_Send(EVT_KEY_RELEASE, (int16_t)i);
                stop_long_press();
            }
        }
    }
    uint8_t raw_enc = DEV_EC11_IsKeyPressed();
    if(raw_enc != s_enc_state)
    {
        s_enc_state = raw_enc;
        if(raw_enc == 1)
        APP_Event_Send(EVT_ENC_PRESS,0);
        else
        APP_Event_Send(EVT_ENC_RELEASE,0);
    }
}

/*读检测线稳定电平，判断插拔发事件(0 = 连接，1 = 断开)*/
static void scan_detect_line(void)
{
    uint8_t raw = (HAL_GPIO_ReadPin(DETECT_PORT,DETECT_PIN) == GPIO_PIN_RESET) ? 0 : 1;
    if(raw != s_detect_state)
    {
        s_detect_state = raw;
        if(raw == 0)
        {
            APP_Event_Send(EVT_DEVICE_PLUG,0);
        }
        else
        {
            APP_Event_Send(EVT_DEVICE_UNPLUG,0);
        }
    }
}

/*编码器旋转(保持10Ms轮询)*/
static void scan_encoder(void)
{
    int16_t step = DEV_EC11_GetStep();
    if(step > 0)
    APP_Event_Send(EVT_ENC_RIGHT,step);
    else if(step < 0)
    APP_Event_Send(EVT_ENC_LEFT,(int16_t)(-step));
}

/*输入任务入口:事件驱动+编码器轮询*/
void TaskInput(void *argument)
{
    (void)argument;
    s_input_task = xTaskGetCurrentTaskHandle();
    /*长按定时器：超时回调发EVT_KEY_LONG*/
    s_long_timer = xTimerCreate("long",pdMS_TO_TICKS(LONG_PRESS_MS),pdFALSE,(void*)0,long_press_cb);
    printf("[INPUT]start\r\n");
    for(;;)
    {
        uint32_t notify = 0;
        BaseType_t ret = xTaskNotifyWait(0x00000000,0xFFFFFFFF,&notify,pdMS_TO_TICKS(ENC_POLL_MS));
        if(ret == pdPASS)
        {
            /*EXTI触发——延时消抖——读稳定电平判断边沿*/
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            if(notify & NOTIFY_KEY) scan_keys();
            if(notify & NOTIFY_DETECT)  scan_detect_line();
        }
        else
        {
            /*超时则编码器旋转*/
            scan_encoder();
        }
    }
}
