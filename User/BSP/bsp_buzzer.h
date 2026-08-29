#ifndef __BSP_BUZZER_H
#define __BSP_BUZZER_H

#include "main.h"

#define BSP_BUZZER_TIM          htim4           /* PWM 定时器 */
#define BSP_BUZZER_CHANNEL      TIM_CHANNEL_4   /* PWM 输出通道 */
#define BSP_BUZZER_CLK_HZ       1000000         /* 定时器计数时钟频率 */

void BSP_Buzzer_Init(void);
void BSP_Buzzer_SetFrequency(uint32_t freq_hz);
void BSP_Buzzer_SetVolume(uint8_t volome);
void BSP_Buzzer_Stop(void);

#endif /* __BSP_BUZZER_H */
