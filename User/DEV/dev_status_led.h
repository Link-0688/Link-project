#ifndef __DEV_STATUS_LED_H
#define __DEV_STATUS_LED_H

typedef enum{
    LED_MODE_OFF = 0,   /*全灭*/
    LED_MODE_GREEN_SOLID,   /*绿灯常亮*/
    LED_MODE_GREEN_BREATH,  /*绿灯呼吸*/
    LED_MODE_RED_FLASH,     /*红灯快闪*/
}led_mode_t;

void DEV_StatusLed_Init(void);
void DEV_StatusLed_SetMode(led_mode_t mode);

#endif /*__DEV_STATUS_LED_H*/
