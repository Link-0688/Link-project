#include "app_event.h"

#define INPUT_QUEUE_LEN 32

QueueHandle_t g_input_queue = NULL;

uint8_t APP_Event_Init(void)
{
    g_input_queue = xQueueCreate(INPUT_QUEUE_LEN,sizeof(input_event_t));
    return (g_input_queue != NULL) ? 1 : 0;
}

uint8_t APP_Event_Send(event_type_t type,int16_t param)
{
    if(g_input_queue == NULL)   return 0;
    input_event_t evt;
    evt.type = type;
    evt.param = param;
    return (xQueueSend(g_input_queue,&evt,0) == pdPASS) ? 1 : 0;
}
