#ifndef __APP_FILE_H
#define __APP_FILE_H

#include "app_event.h"

void APP_File_Init(void);
void APP_File_Exit(void);
void APP_File_HandleEvent(input_event_t *p_evt);
void APP_File_Render(void);

#endif /*__APP_FILE_H*/
