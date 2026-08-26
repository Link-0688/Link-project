#ifndef __APP_SETTING_H
#define __APP_SETTING_H

#include "app_event.h"

void APP_Setting_Init(void);
void APP_Setting_Exit(void);
void APP_Setting_HandleEvent(input_event_t *p_evt);
void APP_Setting_Render(void);

#endif /*__APP_Setting_H*/
