#include "bsp_spi.h"

void BSP_SPI2_Init(void)
{
    SPI2_CS_HIGH();
    __HAL_SPI_ENABLE(&hspi2);
}

/**
 * @brief SPI2 收发一字节（寄存器级，无 HAL 开销，最可靠）
 */
uint8_t BSP_SPI2_ReadWriteByte(uint8_t tx_data)
{
    while (!(hspi2.Instance->SR & SPI_SR_TXE));
    *(volatile uint8_t *)&hspi2.Instance->DR = tx_data;
    while (!(hspi2.Instance->SR & SPI_SR_RXNE));
    return *(volatile uint8_t *)&hspi2.Instance->DR;
}
