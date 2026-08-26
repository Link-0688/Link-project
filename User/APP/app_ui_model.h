#ifndef __APP_UI_MODEL_H
#define __APP_UI_MODEL_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

/*系统状态*/
typedef enum{
    SYS_STATE_BOOT = 0,     /*启动界面*/
    SYS_STATE_LOGIN,        /*登录界面*/
    SYS_STATE_LOCK,         /*锁定*/
    SYS_STATE_DESKTOP,      /*桌面*/
    SYS_STATE_APP_FILE,     /*文件管理*/
    SYS_STATE_APP_DRAW,     /*画图*/
    SYS_STATE_APP_MUSIC,    /*音乐*/
    SYS_STATE_APP_LOG,      /*日志*/
    SYS_STATE_APP_MONITOR,  /*监控*/
    SYS_STATE_APP_SETTING,  /*设置*/
    SYS_STATE_SCREEN_OFF,   /*熄屏*/
    SYS_STATE_NUM
}sys_state_t;

#define DESKTOP_ICON_NUM    6   /*icon:图标*/

/*界面模型：TaskSys写，TaskUI读，互斥锁保护*/
typedef struct{
    sys_state_t state;  /*当前状态*/
    sys_state_t state_before_sleep; /*熄屏前状态*/
    /*桌面*/
    uint8_t cursor_index;   /*选中图标0~5*/
    /*登录*/
    uint8_t pwd_input[4];   /*已输入数字(1~4)*/
    uint8_t pwd_len;        /*已输入位数*/
    uint8_t pwd_error;      /*1=显示ERROR*/
    uint8_t lock_remain_s;  /*锁定剩余秒数*/
    uint8_t error_count;    /*连续错误次数*/
    /*设备*/
    uint8_t device_connected;/*1=已连接*/
    /*时间（秒级刷新）*/
    uint8_t hour,minute,second;
    uint8_t dirty;  /*界面变化标志*/
}ui_model_t;

extern ui_model_t g_ui_model;
extern SemaphoreHandle_t g_ui_mutex;

void APP_UIModel_Init(void);
void APP_UIModel_Lock(void);
void APP_UIModel_Unlock(void);

#endif /*__APP_UI_MODEL_H*/
