#include "dev_config.h"
#include "ff.h"
#include <string.h>

#define CONFIG_FILE_NAME    "CONFIG.BIN"

static config_t s_config;
static const config_t  s_default_config =
{
    .sensitivity = 3,
    .cursor_size = 8,
    .brightness  = 80,
    .volume      = 50,
    .sleep_time  = 30,
};

void DEV_Config_Init(void)
{
    s_config = s_default_config;
}

uint8_t DEV_Config_Load(void)
{
    FIL file;
    UINT br = 0;

    if(f_open(&file,CONFIG_FILE_NAME,FA_READ) != FR_OK)     return 0;
    if(f_read(&file,&s_config,sizeof(config_t),&br) != FR_OK || br != sizeof(config_t))
    {
        f_close(&file);
        return 0;
    }
    f_close(&file);
    return 1;
}

uint8_t DEV_Config_Save(void)
{
    FIL file;
    UINT bw = 0;
    if(f_open(&file,CONFIG_FILE_NAME,FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) return 0;
    if(f_write(&file,&s_config,sizeof(config_t),&bw) != FR_OK || bw != sizeof(config_t))
    {
        f_close(&file);
        return 0;
    }
    f_close(&file);
    return 1;
}

config_t DEV_Config_Get(void)
{
    return s_config;
}

void DEV_Config_Set(const config_t *p_cfg)
{
    s_config = *p_cfg;
}
