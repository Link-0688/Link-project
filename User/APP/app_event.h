#ifndef __APP_EVENT_H
#define __APP_EVENT_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

typedef enum{
    EVT_NONE = 0,
    EVT_KEY_PRESS,      /*按下,1*/
    EVT_KEY_RELEASE,    /*松开,2*/
    EVT_KEY_LONG,       /*长按,3*/
    EVT_ENC_LEFT,       /*左转,4*/
    EVT_ENC_RIGHT,      /*右转,5*/
    EVT_ENC_PRESS,      /*按下,6*/
    EVT_ENC_RELEASE,    /*松开,7*/
    EVT_DEVICE_PLUG,    /*插入,8*/
    EVT_DEVICE_UNPLUG,  /*拔出,9*/
    EVT_NUM             /*10*/
}event_type_t;

typedef struct{
    event_type_t type;
    int16_t      param;
}input_event_t;

extern QueueHandle_t g_input_queue;

uint8_t APP_Event_Init(void);
uint8_t APP_Event_Send(event_type_t type,int16_t param);    /*入队，满返回0*/

#endif /*__APP_EVENT_H*/
