#include "bsp_led.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;    
}led_gpio_t;

static const led_gpio_t s_led_map[LED_NUM] =
{
    {GPIOD,GPIO_PIN_13},    /*LED_RED = PD13*/
    {GPIOD,GPIO_PIN_14},    /*LED_GREEN = PD14*/
};

#define LED_COMMON_PORT GPIOB
#define LED_COMMON_PIN  GPIO_PIN_4

void BSP_LED_Init(void)
{
    HAL_GPIO_WritePin(LED_COMMON_PORT,LED_COMMON_PIN,GPIO_PIN_RESET); /*默认关闭*/
    BSP_LED_Set(LED_RED,0);
    BSP_LED_Set(LED_GREEN,0);
}

void BSP_LED_Set(led_channel_t ch,uint8_t on)
{
    if(ch >= LED_NUM) return;
    GPIO_PinState state = on ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(s_led_map[ch].port,s_led_map[ch].pin,state);
}
