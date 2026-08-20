#ifndef __DEV_EC11_H
#define __DEV_EC11_H

#include "main.h"
#include "bsp_encoder.h"
#include <stdint.h>

#define EC11_KEY_PORT   GPIOA
#define EC11_KEY_PIN    GPIO_PIN_11
/*1格对应4个边沿脉冲*/
#define EC11_PULSE_PER_NOTCH    4

void DEV_EC11_Init(void);
int16_t DEV_EC11_GetStep(void); /*本次步进与上次的差值(单位：格)*/
uint8_t DEV_EC11_IsKeyPressed(void);    /*编码器按键原始电平*/

#endif /*__DEV_EC11_H*/
