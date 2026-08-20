#include "bsp_encoder.h"

void BSP_Encoder_Init(void)
{
    HAL_TIM_Encoder_Start(&BSP_ENC_TIM,TIM_CHANNEL_ALL);
    BSP_ENC_TIM.Instance->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E);  /*确保CCR两个通道都打开*/
}

uint16_t BSP_Encoder_GetRawCount(void)
{
    return (uint16_t)__HAL_TIM_GET_COUNTER(&BSP_ENC_TIM);   /*&htim2在SRAM的首地址*/
    /*本质是在访问htim2.Instance->CNT,所以拿到的值是CNT的值*/
}
