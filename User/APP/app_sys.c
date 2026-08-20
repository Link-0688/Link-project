#include "app_event.h"
#include "app_ui_model.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "dev_spi_oled.h"
#include <stdio.h>

#define PASSWORD_LEN    4

static const uint8_t s_password[PASSWORD_LEN] = {1,2,2,4};

static TickType_t s_last_active_tick = 0;
static TickType_t s_lock_start_tick = 0;

/*工具：读当前状态*/
static sys_state_t get_state(void)
{
    sys_state_t s;
    APP_UIModel_Lock();
    s = g_ui_model.state;
    APP_UIModel_Unlock();   //给互斥锁保证赋值成功
    return s;
}

/*更新设备状态LED*/
static void update_device_led(uint8_t connected)
{
    if(connected)
    {
        BSP_LED_Set(LED_GREEN,1);
        BSP_LED_Set(LED_RED,0);
    }
    else
    {
        BSP_LED_Set(LED_GREEN,0);
        BSP_LED_Set(LED_RED,1);
    }
}

/*熄屏/唤醒*/
static void enter_screen_off(void)
{
    APP_UIModel_Lock();
    g_ui_model.state_before_sleep = g_ui_model.state;
    g_ui_model.state = SYS_STATE_SCREEN_OFF;
    g_ui_model.dirty = 1;
    APP_UIModel_Unlock();
    dev_oled_display_off();
}

static void wake_up(void)
{
    APP_UIModel_Lock();
    g_ui_model.state = g_ui_model.state_before_sleep;
    g_ui_model.dirty = 1;
    APP_UIModel_Unlock();
    dev_oled_display_on();
}

/*登录处理*/
static void enter_login(void)
{
    APP_UIModel_Lock();
    g_ui_model.state = SYS_STATE_LOGIN;
    g_ui_model.pwd_len = 0;
    g_ui_model.pwd_error = 0;
    g_ui_model.dirty = 1;
    APP_UIModel_Unlock();
}

static void process_login(input_event_t *p_evt)
{
    if(p_evt->type != EVT_KEY_PRESS)    return;
    if(p_evt->param < KEY_1 || p_evt->param > KEY_4)    return;
    uint8_t digit = (uint8_t)(p_evt->param + 1);    /*KEY_1->数字1*/
    APP_UIModel_Lock();
    g_ui_model.pwd_input[g_ui_model.pwd_len] = digit;
    g_ui_model.pwd_len ++;
    g_ui_model.dirty = 1;
    
    if(g_ui_model.pwd_len >= PASSWORD_LEN)
    {
        uint8_t ok = 1;
        for(uint8_t i = 0; i < PASSWORD_LEN; i++)
        {
            if(g_ui_model.pwd_input[i] != s_password[i])
            {
                ok = 0;
                break;
            }
        }
        if(ok)
        {
            g_ui_model.pwd_len = 0;
            g_ui_model.error_count = 0;
            g_ui_model.state = SYS_STATE_DESKTOP;
            g_ui_model.cursor_index = 0;
        }
        else
        {
            g_ui_model.pwd_len = 0;
            g_ui_model.pwd_error = 1;
            g_ui_model.error_count ++;
            if(g_ui_model.error_count >= 3)
            {
                g_ui_model.state = SYS_STATE_LOCK;
                g_ui_model.lock_remain_s = 5;
                g_ui_model.error_count = 0;
                s_lock_start_tick = xTaskGetTickCount();
            }
        }
    }
    APP_UIModel_Unlock();
}

/*桌面处理*/
static void process_desktop(input_event_t *p_evt)
{
    if(p_evt->type == EVT_ENC_LEFT)
    {
        APP_UIModel_Lock();
        if(g_ui_model.cursor_index > 0)
        {
            g_ui_model.cursor_index--;
        }
        else
        {
            g_ui_model.cursor_index = DESKTOP_ICON_NUM - 1; //切回最下方那一个
        }
        g_ui_model.dirty = 1;
        APP_UIModel_Unlock();
    }
    else if(p_evt->type == EVT_ENC_RIGHT)
    {
        APP_UIModel_Lock();
        g_ui_model.cursor_index = (g_ui_model.cursor_index + 1) % DESKTOP_ICON_NUM;
        g_ui_model.dirty = 1;
        APP_UIModel_Unlock();
    }
    else if(p_evt->type == EVT_ENC_PRESS)
    {
        printf("[SYS] open icon %d\r\n",g_ui_model.cursor_index);
    }
}

/*事件分发*/
static void process_event(input_event_t *p_evt)
{
    sys_state_t state = get_state();
    /*全局熄屏：非熄屏状态下长按KEY4*/
    if(state != SYS_STATE_SCREEN_OFF && p_evt->type == EVT_KEY_LONG && p_evt->param == KEY_4)
    {
        enter_screen_off();
        return;
    }
    switch(state)
    {
        case SYS_STATE_BOOT : if(p_evt->type == EVT_KEY_LONG && p_evt->param == KEY_2) enter_login(); break;
        case SYS_STATE_LOGIN : process_login(p_evt); break;
        case SYS_STATE_LOCK : break;    /*锁定期间忽略输入*/
        case SYS_STATE_DESKTOP : process_desktop(p_evt); break;
        case SYS_STATE_SCREEN_OFF : if(p_evt->type == EVT_KEY_LONG && p_evt->param == KEY_1) wake_up(); break;
        default : break;
    }
}

/*1秒无事件时的周期处理*/
static void process_timeout(void)
{
    sys_state_t state = get_state();
    /*锁定倒计时*/
    if(state == SYS_STATE_LOCK)
    {
        uint8_t remain;
        APP_UIModel_Lock();
        if(g_ui_model.lock_remain_s > 0)
        g_ui_model.lock_remain_s--;
        remain = g_ui_model.lock_remain_s;
        g_ui_model.dirty = 1;
        APP_UIModel_Unlock();
        if(remain == 0)
        {
            APP_UIModel_Lock();
            g_ui_model.state = SYS_STATE_LOGIN;
            g_ui_model.pwd_len = 0;
            g_ui_model.dirty = 1;
            APP_UIModel_Unlock();
        }
    return;
    }
    /*超时熄屏*/
    if(state != SYS_STATE_SCREEN_OFF)
    {
        uint16_t sleep_s = 30;  /*默认30秒*/
        if((xTaskGetTickCount() - s_last_active_tick) >= pdMS_TO_TICKS(sleep_s * 1000))
        enter_screen_off();
    }
}

/*系统状态机任务入口*/
void TaskSys(void *argument)
{
    (void)argument;
    APP_UIModel_Init();
    BSP_LED_Init();
    update_device_led(1);
    s_last_active_tick = xTaskGetTickCount();
    printf("[SYS] start\r\n");
    for(;;)
    {
        input_event_t evt;
        BaseType_t ret = xQueueReceive(g_input_queue,&evt,pdMS_TO_TICKS(1000));
        if(ret == pdPASS)
        {
            s_last_active_tick = xTaskGetTickCount();
            process_event(&evt);
        }
        else
        {
            process_timeout();
        }
    }
}
