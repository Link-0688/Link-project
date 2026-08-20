/*keil5用fputc,Cmake要求用——write*/
#include "bsp_uart.h"
#include "usart.h"

/* GCC(newlib) 下 printf 重定向到 USART1 */
int _write(int fd, char *p_data, int len)
{
    (void)fd;
    HAL_UART_Transmit(&huart1, (uint8_t *)p_data, (uint16_t)len, HAL_MAX_DELAY);
    return len;
}
