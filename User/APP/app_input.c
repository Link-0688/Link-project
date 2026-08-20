#include "app_event.h"
#include "app_input.h"
#include "dev_ec11.h"
#include "bsp_key.h"
#include <stdio.h>

/*检测线(判断输入设备是否连接成功)*/
#define DETECT_PORT     GPIOC
#define DETECT_PIN      GPIO_PIN_5

/*扫描与消抖参数*/
#define SCAN_PERIOD_MS  10  /*扫描周期(任务循环)*/
#define DEBOUNCE_MS     20  /*消抖时间*/
#define LONG_PRESS_MS   2000/*长按时间*/

/*按键消抖状态机*/
typedef struct{
    uint8_t raw_last;   /*上次采样*/
    uint8_t cnt;        /*连续相同计数*/
    uint8_t state;      /*消抖后稳定状态0/1*/
    uint16_t hole_ms;   /*按住时长*/
    uint8_t long_sent;  /*长按是否上报*/
}key_state_t;

static key_state_t s_keys[KEY_NUM];     /*4个板载按键*/
static key_state_t s_enc_key;           /*编码器按键*/
static key_state_t s_detect;            /*检测线*/
static uint8_t  s_device_connected = 1; /*当前是否已连接*/

/*统一消抖*/
static void scan_one_key(key_state_t *p_k, uint8_t raw, event_type_t press_evt,
event_type_t release_evt, event_type_t long_evt, int16_t param)
{
    if(raw == p_k->raw_last)
    {
        if(p_k->cnt < 255)  p_k->cnt++;
    }
    else
    {
        p_k->raw_last = raw;
        p_k->cnt = 0;
        return;
    }
    /*稳定时间足够，判定边沿*/
    if((uint16_t)p_k->cnt * SCAN_PERIOD_MS >= DEBOUNCE_MS)
    {
        if(p_k->state == 0 && raw == 1)
        {
            p_k->state = 1;
            p_k->hole_ms = 0;
            p_k->long_sent = 0;
            APP_Event_Send(press_evt,param);
        }
        else if(p_k->state == 1 && raw == 0)
        {
            p_k->state = 0;
            APP_Event_Send(release_evt,param);
        }
    }
    /*长按检测*/
    if(p_k->state == 1)
    {
        p_k->hole_ms += SCAN_PERIOD_MS;
        if(p_k->hole_ms >= LONG_PRESS_MS && !p_k->long_sent)
        {
            p_k->long_sent = 1;
            APP_Event_Send(long_evt,param);
        }
    }
}
/*检测线扫描 0 = 连接，1 = 断开*/
static void scan_detect_line(void)
{
    uint8_t raw = (HAL_GPIO_ReadPin(DETECT_PORT,DETECT_PIN) == GPIO_PIN_RESET) ? 0 : 1;
    /*复用消抖状态机，只关心稳定后的边沿*/
    if(raw == s_detect.raw_last)
    {
        if(s_detect.cnt < 255)  s_detect.cnt++;
    }
    else
    {
        s_detect.raw_last = raw;
        s_detect.cnt = 0;
        return;
    }
    if((uint16_t)s_detect.cnt * SCAN_PERIOD_MS >= DEBOUNCE_MS)
        {
            if(s_detect.state != raw)
            {
                s_detect.state = raw;
                if(raw == 0)
                {
                    s_device_connected = 1;
                    APP_Event_Send(EVT_DEVICE_PLUG,0);
                }
                else
                {
                    s_device_connected = 0;
                    APP_Event_Send(EVT_DEVICE_UNPLUG,0);
                }
            }
        }
}
/*输入扫描入口*/
void TaskInput(void *argument)
{
    (void)argument;
    printf("[INPUT] start\r\n");
    for(;;)
    {
    /*1.4个板载按键*/
    scan_one_key(&s_keys[KEY_1],BSP_Key_Read(KEY_1),EVT_KEY_PRESS,EVT_KEY_RELEASE,EVT_KEY_LONG,KEY_1);
    scan_one_key(&s_keys[KEY_2],BSP_Key_Read(KEY_2),EVT_KEY_PRESS,EVT_KEY_RELEASE,EVT_KEY_LONG,KEY_2);
    scan_one_key(&s_keys[KEY_3],BSP_Key_Read(KEY_3),EVT_KEY_PRESS,EVT_KEY_RELEASE,EVT_KEY_LONG,KEY_3);
    scan_one_key(&s_keys[KEY_4],BSP_Key_Read(KEY_4),EVT_KEY_PRESS,EVT_KEY_RELEASE,EVT_KEY_LONG,KEY_4);

    /*2.编码器旋转*/
    int16_t s16_step = DEV_EC11_GetStep();
    if(s16_step > 0)    APP_Event_Send(EVT_ENC_RIGHT, s16_step);
    else if(s16_step < 0) APP_Event_Send(EVT_ENC_LEFT, (int16_t)(-s16_step));

    /*3.编码器按键*/
    scan_one_key(&s_enc_key,DEV_EC11_IsKeyPressed(),EVT_ENC_PRESS,EVT_ENC_RELEASE,EVT_NONE,0);

    /*4.检测线*/
    scan_detect_line();
    vTaskDelay(pdMS_TO_TICKS(SCAN_PERIOD_MS));
    }
}
