#ifndef __APP_DRAW_H
#define __APP_DRAW_H

#include "app_event.h"

void APP_Draw_Init(void);
void APP_Draw_Exit(void);
void APP_Draw_HandleEvent(input_event_t *p_evt);
void APP_Draw_Render(void);

#endif /*__APP_DRAW_H*/
