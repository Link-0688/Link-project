#include "dev_ec11.h"

static uint16_t s_last_count = 0;

void DEV_EC11_Init(void)
{
    BSP_Encoder_Init();
    s_last_count = BSP_Encoder_GetRawCount();
}

int16_t DEV_EC11_GetStep(void)
{
    uint16_t u16_curr = BSP_Encoder_GetRawCount();
    int32_t s32_diff = (int32_t)u16_curr - (int32_t)s_last_count;/*强制类型转换，高位自动填充为0*/
    int16_t s16_step = (int16_t)(s32_diff / (int32_t)EC11_PULSE_PER_NOTCH); //获得步进格数

    /*异常跳变：视为噪声，重置基线丢弃*/
    if(s16_step > 20 || s16_step < -20)
    {
        s_last_count = u16_curr;
        return 0;
    }

    /*正常范围限幅：10ms周期内人手最多约3格*/
    if(s16_step > 3) s16_step = 3;
    if(s16_step < -3)s16_step = -3;
    if(s16_step != 0)
    {
        s_last_count = (uint16_t)(s_last_count + s16_step * EC11_PULSE_PER_NOTCH);
    }
    return s16_step;
}

uint8_t DEV_EC11_IsKeyPressed(void)
{
    return (HAL_GPIO_ReadPin(EC11_KEY_PORT,EC11_KEY_PIN) == GPIO_PIN_RESET) ? 1 : 0;
}
