/**********************************************************
 * 文件名: bsp_sw_spi.h
 * 日  期: 2026-07-31
 *
 * 功能说明:
 *   软件 SPI 基础时序层 (BSP 层)
 *   用于 ELC-F4 开发板板载 SPI OLED (HS96L01W4S03)
 *   仅实现 MOSI 发送, 无 MISO 接收 (OLED 是单向写入)
 *
 * 引脚: PE9=SCK, PE10=MOSI, PE11=RES, PE12=DC, PE13=CS
 *
 * 依赖: main.h (HAL 库)
 * 架构分层: APP → DEV → BSP (本层) → HAL
 **********************************************************/

#ifndef __BSP_SW_SPI_H
#define __BSP_SW_SPI_H

#include "main.h"
#include <stdint.h>

/* ---- 引脚宏 (ELC-F4 板载 SPI OLED) ----
   PE9  = SCL = SCK  = D0  (时钟线)
   PE10 = SDA = MOSI = D1  (数据线)
   PE11 = RES              (复位)
   PE12 = DC               (数据/命令选择)
   PE13 = CS               (片选)
------------------------------------------- */
#define BSP_SPI_SCK_PORT    GPIOE
#define BSP_SPI_SCK_PIN     GPIO_PIN_9   /* PE9  = SCL */

#define BSP_SPI_MOSI_PORT   GPIOE
#define BSP_SPI_MOSI_PIN    GPIO_PIN_10  /* PE10 = SDA */

#define BSP_SPI_RES_PORT    GPIOE
#define BSP_SPI_RES_PIN     GPIO_PIN_11  /* PE11 = RES */

#define BSP_SPI_DC_PORT     GPIOE
#define BSP_SPI_DC_PIN      GPIO_PIN_12  /* PE12 = DC */

#define BSP_SPI_CS_PORT     GPIOE
#define BSP_SPI_CS_PIN      GPIO_PIN_13  /* PE13 = CS */

/* ---- API ---- */
void bsp_sw_spi_init(void);
void bsp_sw_spi_cs_low(void);
void bsp_sw_spi_cs_high(void);
void bsp_sw_spi_dc_low(void);
void bsp_sw_spi_dc_high(void);
void bsp_sw_spi_res_low(void);
void bsp_sw_spi_res_high(void);
void bsp_sw_spi_write_byte(uint8_t u8_data);

#endif /* __BSP_SW_SPI_H */
