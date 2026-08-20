#include "app_ui_model.h"

ui_model_t g_ui_model;
SemaphoreHandle_t g_ui_mutex = NULL;

void APP_UIModel_Init(void)
{
    g_ui_mutex = xSemaphoreCreateMutex();
    g_ui_model.state = SYS_STATE_BOOT;
    g_ui_model.state_before_sleep = SYS_STATE_BOOT;
    g_ui_model.cursor_index = 0;
    g_ui_model.pwd_len = 0;
    g_ui_model.pwd_error = 0;
    g_ui_model.error_count = 0;
    g_ui_model.device_connected = 1;
    g_ui_model.dirty = 1;
}

void APP_UIModel_Lock(void)
{
    if(g_ui_mutex != NULL)
    {
        xSemaphoreTake(g_ui_mutex,portMAX_DELAY);
    }
}

void APP_UIModel_Unlock(void)
{
    if(g_ui_mutex != NULL)
    {
        xSemaphoreGive(g_ui_mutex);
    }
}
