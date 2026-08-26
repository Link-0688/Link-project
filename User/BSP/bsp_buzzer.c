#include "bsp_buzzer.h"
#include "tim.h"

void BSP_Buzzer_Init(void)
{
    HAL_TIM_PWM_Stop(&BSP_BUZZER_TIM, BSP_BUZZER_CHANNEL);
}

void BSP_Buzzer_SetFrequency(uint32_t freq_hz)
{
    if (freq_hz == 0)
    {
        BSP_Buzzer_Stop();
        return;
    }

    uint32_t arr = (BSP_BUZZER_CLK_HZ / freq_hz) - 1;
    uint32_t ccr = arr / 2;

    __HAL_TIM_SET_AUTORELOAD(&BSP_BUZZER_TIM, arr);
    __HAL_TIM_SET_COMPARE(&BSP_BUZZER_TIM, BSP_BUZZER_CHANNEL, ccr);

    HAL_TIM_PWM_Start(&BSP_BUZZER_TIM, BSP_BUZZER_CHANNEL);
}

void BSP_Buzzer_Stop(void)
{
    HAL_TIM_PWM_Stop(&BSP_BUZZER_TIM, BSP_BUZZER_CHANNEL);
}
