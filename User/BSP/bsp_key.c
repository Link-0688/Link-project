#include "bsp_key.h"

typedef struct{
    GPIO_TypeDef    *port;
    uint16_t         pin;
}key_gpio_t;

static const key_gpio_t s_key_map[KEY_NUM] = {
    {GPIOB,GPIO_PIN_5},     /*KEY_1*/
    {GPIOB,GPIO_PIN_6},     /*KEY_2*/
    {GPIOB,GPIO_PIN_7},     /*KEY_3*/
    {GPIOB,GPIO_PIN_8},     /*KEY_4*/
};

void BSP_Key_Init(void)
{
    /*已经在Cubemx做好初始化，无需再写代码*/
}

uint8_t BSP_Key_Read(key_id_t id)
{
    if(id >= KEY_NUM)   return 0;   /*兜底*/
    return (HAL_GPIO_ReadPin(s_key_map[id].port,s_key_map[id].pin) == GPIO_PIN_RESET) ? 1 : 0;
}
