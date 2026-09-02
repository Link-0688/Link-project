#include "dev_status_led.h"
#include "bsp_led.h"
#include "FreeRTOS.h"
#include "timers.h"

#define BREATH_PERIOD_MS    3000    /*呼吸周期3秒*/
#define PWM_TICK_MS 1   /*软件PWM tick = 1ms*/
#define PWM_CARRIER 10  /*载波周期 = 10ms(100hz)*/

static TimerHandle_t s_timer = NULL;
static volatile led_mode_t s_mode = LED_MODE_OFF;
static uint32_t s_phase = 0;    /*相位计数，每tick + 1*/
static uint8_t  s_pwm_cnt = 0;   /*载波相位 0~PWM_CARRIER - 1*/

/*由相位求呼吸亮度百分比(三角波0~100)*/
static uint8_t breath_brightness(uint32_t phase)
{
    uint32_t p = phase % BREATH_PERIOD_MS;
    if(p < BREATH_PERIOD_MS / 2)
    {
        return (uint8_t)(p * 100 / (BREATH_PERIOD_MS / 2));
    }
    return (uint8_t)((BREATH_PERIOD_MS - p) * 100 / (BREATH_PERIOD_MS / 2));
}

static void timer_cb(TimerHandle_t  xTimer)
{
    (void)xTimer;
    uint8_t green_on = 0;
    uint8_t red_on = 0;

    switch(s_mode)
    {
        case LED_MODE_OFF : green_on = 0; red_on = 0; break;
        case LED_MODE_GREEN_SOLID : green_on = 1; red_on = 0; break;
        case LED_MODE_RED_SOLID : green_on = 0; red_on = 1; break;
        case LED_MODE_GREEN_BREATH : 
        {
            uint8_t duty = breath_brightness(s_phase);  /*0~100*/
            uint8_t level = (uint8_t)(duty / 10);   /*0~10*/
            green_on = (s_pwm_cnt < level) ? 1 : 0;
            red_on = 0;
            break;
        }
        case LED_MODE_RED_FLASH : red_on = ((s_phase % 1000) < 500) ? 1 : 0; green_on = 0; break;
        default : break;
    }
    BSP_LED_Set(LED_GREEN,green_on);
    BSP_LED_Set(LED_RED,red_on);
    s_phase++;
    s_pwm_cnt = (uint8_t)((s_pwm_cnt + 1) % PWM_CARRIER);
}

void DEV_StatusLed_Init(void)
{
    BSP_LED_Init();
    s_mode = LED_MODE_GREEN_SOLID;
    s_phase = 0;
    s_pwm_cnt = 0;
    s_timer = xTimerCreate("led",pdMS_TO_TICKS(PWM_TICK_MS),pdTRUE,(void*)0,timer_cb);
    if(s_timer != NULL)
    {
        xTimerStart(s_timer,0);
    }
}

void DEV_StatusLed_SetMode(led_mode_t mode)
{
    s_mode = mode;
    /*常亮/全灭模式立即刷新*/
    if(mode == LED_MODE_OFF || mode == LED_MODE_GREEN_SOLID || mode == LED_MODE_RED_SOLID)
    {
        BSP_LED_Set(LED_GREEN,(mode == LED_MODE_GREEN_SOLID) ? 1 : 0);
        BSP_LED_Set(LED_RED,(mode == LED_MODE_RED_SOLID) ? 1 : 0);
    }
}
