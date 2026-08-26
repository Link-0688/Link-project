#ifndef __APP_MONITOR_H
#define __APP_MONITOR_H

#include "app_event.h"

void APP_Monitor_Init(void);
void APP_Monitor_Exit(void);
void APP_Monitor_HandleEvent(input_event_t *p_evt);
void APP_Monitor_Render(void);

#endif /*__APP_MONITOR_H*/
