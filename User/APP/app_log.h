#ifndef __APP_LOG_H
#define __APP_LOG_H

#include "app_event.h"

void APP_Log_Init(void);
void APP_Log_Open(void);
void APP_Log_Exit(void);
void APP_Log_HandleEvent(input_event_t *p_evt);
void APP_Log_Render(void);
void APP_Log_Add(const char *msg);
void APP_Log_Clear(void);   
void APP_Log_Dump(void);    /*通过串口打印全部日志(CLI用)*/

#endif  /*__APP_LOG_H*/
