#ifndef __DEV_CONFIG_H
#define __DEV_CONFIG_H

#include <stdint.h>

typedef struct{
    uint8_t sensitivity;    /*光标灵敏度 1~10*/
    uint8_t cursor_size;    /*光标大小*/
    uint8_t brightness;     /*亮度0~100*/
    uint8_t volume;         /*音量0~100*/
    uint16_t sleep_time;    /*熄屏时间(秒)*/
}config_t;

void DEV_Config_Init(void);
uint8_t DEV_Config_Load(void);  /*从SD读，成功返回1*/
uint8_t DEV_Config_Save(void);  /*写SD,成功返回1*/
config_t DEV_Config_Get(void);
void DEV_Config_Set(const config_t *p_cfg);

#endif /*__DEV_CONFIG_H*/
