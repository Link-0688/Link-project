#ifndef __APP_LOG_H
#define __APP_LOG_H

#include "app_event.h"

void APP_Log_Init(void);
void APP_Log_Open(void);
void APP_Log_Exit(void);
void APP_Log_HandleEvent(input_event_t *p_evt);
void APP_Log_Render(void);
void APP_Log_Add(const char *msg);

#endif  /*__APP_LOG_H*/
