#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "main.h"
#include "spi.h"

#define SPI2_CS_PIN      GPIO_PIN_12
#define SPI2_CS_PORT     GPIOB

#define SPI2_CS_LOW()    HAL_GPIO_WritePin(SPI2_CS_PORT, SPI2_CS_PIN, GPIO_PIN_RESET)
#define SPI2_CS_HIGH()   HAL_GPIO_WritePin(SPI2_CS_PORT, SPI2_CS_PIN, GPIO_PIN_SET)

void    BSP_SPI2_Init(void);
uint8_t BSP_SPI2_ReadWriteByte(uint8_t tx_data);

#endif
