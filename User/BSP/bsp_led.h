#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "main.h"
#include <stdint.h>

typedef enum {
    LED_RED = 0,
    LED_GREEN,
    LED_NUM
} led_channel_t;

void BSP_LED_Init(void);
void BSP_LED_Set(led_channel_t ch, uint8_t on);   /* on=1 点亮 */

#endif
