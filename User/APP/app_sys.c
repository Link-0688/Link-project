#include "app_event.h"
#include "app_ui_model.h"
#include "app_health.h"
#include "bsp_key.h"
#include "dev_status_led.h"
#include "dev_spi_oled.h"
#include <stdio.h>

/*第三阶段实现功能新增内容*/
#include "app_file.h"
#include "app_draw.h"
#include "app_music.h"
#include "app_log.h"
#include "app_monitor.h"
#include "app_setting.h"
#include "dev_sd.h"
#include "dev_config.h"
#include "fatfs.h"

static uint8_t s_sd_ready = 0;
uint8_t APP_Sys_IsSdReady(void)
{
    return s_sd_ready;
}

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
/*状态正常：绿灯亮  状态异常：红灯亮*/
static void update_device_led(uint8_t connected)
{
    if(connected)
    {
        DEV_StatusLed_SetMode(LED_MODE_GREEN_BREATH);   /* 正常: 绿灯呼吸 */
    }
    else
    {
        DEV_StatusLed_SetMode(LED_MODE_RED_FLASH);      /* 异常: 红灯快闪 */
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
    DEV_StatusLed_SetMode(LED_MODE_OFF);
    APP_Log_Add("SLEEP");
}

static void wake_up(void)
{
    APP_UIModel_Lock();
    g_ui_model.state = g_ui_model.state_before_sleep;
    g_ui_model.dirty = 1;
    APP_UIModel_Unlock();
    dev_oled_display_on();
    update_device_led(g_ui_model.device_connected); 
    APP_Log_Add("WAKE");
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
    const char *log_msg = NULL;    /*解锁后再记录，避免持锁时重入死锁*/
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
            log_msg = "LOGIN OK";
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
            log_msg = "LOGIN FAIL";
        }
    }
    APP_UIModel_Unlock();

    if(log_msg != NULL)
    {
        APP_Log_Add(log_msg);
    }
}

/*根据图表索引打开引用*/
static void open_app(uint8_t index)
{
    APP_UIModel_Lock();
    switch(index){
        case 0 : g_ui_model.state = SYS_STATE_APP_FILE; APP_File_Init(); break;
        case 1 : g_ui_model.state = SYS_STATE_APP_DRAW; APP_Draw_Init(); break;
        case 2 : g_ui_model.state = SYS_STATE_APP_MUSIC; APP_Music_Init(); break;
        case 3 : g_ui_model.state = SYS_STATE_APP_LOG; APP_Log_Open(); break;
        case 4 : g_ui_model.state = SYS_STATE_APP_MONITOR; APP_Monitor_Init(); break;
        case 5 : g_ui_model.state = SYS_STATE_APP_SETTING; APP_Setting_Init(); break;
        default : break;
    }
    g_ui_model.dirty = 1;
    APP_UIModel_Unlock();
    APP_Log_Add("OPEN APP");
}

/*退出当前应用，回桌面*/
static void exit_app(void)
{
    sys_state_t state;
    APP_UIModel_Lock();
    state = g_ui_model.state;
    /*先切回桌面并置脏，让 TaskUI 立即响应，避免保存 SD 卡阻塞造成"1秒才返回"*/
    g_ui_model.state = SYS_STATE_DESKTOP;
    g_ui_model.dirty = 1;
    APP_UIModel_Unlock();

    switch(state)
    {
        case SYS_STATE_APP_FILE : APP_File_Exit();  break;
        case SYS_STATE_APP_DRAW : APP_Draw_Exit(); break;
        case SYS_STATE_APP_MUSIC : APP_Music_Exit(); break;
        case SYS_STATE_APP_LOG : APP_Log_Exit(); break;
        case SYS_STATE_APP_MONITOR : APP_Monitor_Exit(); break;
        case SYS_STATE_APP_SETTING : APP_Setting_Exit(); break;
        default : break;
    }
}

/*把事件转发给当前应用*/
static void app_handle_event(sys_state_t state,input_event_t *p_evt)
{
    switch(state){
        case SYS_STATE_APP_FILE : APP_File_HandleEvent(p_evt);  break;
        case SYS_STATE_APP_DRAW : APP_Draw_HandleEvent(p_evt);  break;
        case SYS_STATE_APP_MUSIC : APP_Music_HandleEvent(p_evt);  break;
        case SYS_STATE_APP_LOG : APP_Log_HandleEvent(p_evt);  break;
        case SYS_STATE_APP_MONITOR : APP_Monitor_HandleEvent(p_evt);  break;
        case SYS_STATE_APP_SETTING : APP_Setting_HandleEvent(p_evt);  break;
        default : break;
    }
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
        uint8_t idx;
        APP_UIModel_Lock();
        idx = g_ui_model.cursor_index;
        APP_UIModel_Unlock();
        open_app(idx);
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

        case SYS_STATE_APP_FILE : 
        case SYS_STATE_APP_DRAW : 
        case SYS_STATE_APP_MUSIC : 
        case SYS_STATE_APP_LOG : 
        case SYS_STATE_APP_MONITOR : 
        case SYS_STATE_APP_SETTING : 
        if(p_evt->type == EVT_ENC_PRESS)    exit_app();
        else    app_handle_event(state,p_evt);

        default : break;
    }

    if(p_evt->type == EVT_DEVICE_PLUG)
    {
        APP_UIModel_Lock();
        g_ui_model.device_connected = 1;
        g_ui_model.dirty = 1;
        APP_UIModel_Unlock();
        update_device_led(1);
        APP_Log_Add("DEV PLUG");
        return;
    }
    if(p_evt->type == EVT_DEVICE_UNPLUG)
    {
        APP_UIModel_Lock();
        g_ui_model.device_connected = 0;
        g_ui_model.dirty = 1;
        APP_UIModel_Unlock();
        update_device_led(0);
        APP_Log_Add("DEV UNPLUG");
        return;
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
        uint16_t sleep_s = DEV_Config_Get().sleep_time;
        if(sleep_s < 5) sleep_s = 5;  /*下限保护，避免误触发*/
        if((xTaskGetTickCount() - s_last_active_tick) >= pdMS_TO_TICKS(sleep_s * 1000))
        {
            enter_screen_off();
        }
    }
}

/*系统状态机任务入口*/
void TaskSys(void *argument)
{
    (void)argument;
    APP_UIModel_Init();
    APP_Log_Init();     /*日志环形缓冲只在上电时初始化一次*/
    DEV_Config_Init();  /*先加载默认配置，后续 DEV_Config_Load 读到合法文件再覆盖*/

    /*SD+FATFS+配置*/
    if(DEV_SD_Init() == 0)
    {
        MX_FATFS_Init();
        if(f_mount(&USERFatFS,"0:",1) == FR_OK)
        {
            s_sd_ready = 1;
            DEV_Config_Load();
            printf("[SYS] SD + FATFS OK\r\n");
        }
        else
        {
            printf("[SYS] f_mount FAIL\r\n");
        }
    }
    else
    {
        printf("[sys] f_mount FAIL\r\n");
    }

    DEV_StatusLed_Init();
    update_device_led(1);
    s_last_active_tick = xTaskGetTickCount();
    printf("[SYS] start\r\n");
    for(;;)
    {
        APP_Health_Beat(HEART_SYS);
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
