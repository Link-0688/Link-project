#ifndef __BSP_ENCODER_H
#define __BSP_ENCODER_H

#include "main.h"
#include "tim.h"
#include <stdint.h>

#define BSP_ENC_TIM htim2   /*编码器：TIM2(CH1 = PA0,CH2 = PA1)*/

void BSP_Encoder_Init(void);
uint16_t BSP_Encoder_GetRawCount(void);

#endif  /*__BSP_ENCODER_H*/
