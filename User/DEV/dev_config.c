#include "dev_config.h"
#include "ff.h"
#include <string.h>

#define CONFIG_FILE_NAME    "CONFIG.BIN"    //config-configuration 配置

static config_t s_config;
static const config_t  s_default_config =
{
    .sensitivity = 3,   //灵敏度
    .cursor_size = 2,   //光标大小(1~4)
    .brightness  = 80,  //亮度
    .volume      = 50,  //音量
    .sleep_time  = 30,  //熄屏时长
};

void DEV_Config_Init(void)
{
    s_config = s_default_config;
}

uint8_t DEV_Config_Load(void)
{
    FIL file;
    UINT br = 0;
    config_t tmp;

    if(f_open(&file,CONFIG_FILE_NAME,FA_READ) != FR_OK)     return 0;
    if(f_read(&file,&tmp,sizeof(config_t),&br) != FR_OK || br != sizeof(config_t))
    {
        f_close(&file);
        return 0;
    }
    f_close(&file);

    /*校验取值范围，防止旧版本/损坏的 CONFIG.BIN 写入非法值（如全 0 导致黑屏）*/
    if(tmp.sensitivity < 1  || tmp.sensitivity > 10 ||
       tmp.cursor_size < 1  || tmp.cursor_size > 4  ||
       tmp.brightness  > 100 || tmp.volume > 100     ||
       tmp.sleep_time  < 5  || tmp.sleep_time > 120)
    {
        return 0;   /*非法数据，保留默认值*/
    }

    s_config = tmp;
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
