#ifndef __APP_MUSIC_H
#define __APP_MUSIC_H

#include "app_event.h"

void APP_Music_Init(void);
void APP_Music_Exit(void);
void APP_Music_HandleEvent(input_event_t *p_evt);
void APP_Music_Render(void);

#endif /*__APP_MUSIC_H*/
