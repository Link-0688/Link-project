/**********************************************************
 * 文件名: bsp_sw_spi.c
 * 日  期: 2026-07-31
 *
 * 功能说明:
 *   软件 SPI 位脉冲实现
 *   SPI Mode 0 (CPOL=0, CPHA=0): SCK 空闲低, 上升沿采样
 *   仅发送 (MOSI), 无需 MISO (OLED 单向写入)
 *
 * 架构分层: APP → DEV → BSP (本层) → HAL
 **********************************************************/

#include "bsp_sw_spi.h"

/* ---- 内部 GPIO 操作宏 ---- */
#define DC_SET()   HAL_GPIO_WritePin(BSP_SPI_DC_PORT,  BSP_SPI_DC_PIN,  GPIO_PIN_SET)
#define DC_CLR()   HAL_GPIO_WritePin(BSP_SPI_DC_PORT,  BSP_SPI_DC_PIN,  GPIO_PIN_RESET)
#define CS_SET()   HAL_GPIO_WritePin(BSP_SPI_CS_PORT,  BSP_SPI_CS_PIN,  GPIO_PIN_SET)
#define CS_CLR()   HAL_GPIO_WritePin(BSP_SPI_CS_PORT,  BSP_SPI_CS_PIN,  GPIO_PIN_RESET)
#define SCK_SET()  HAL_GPIO_WritePin(BSP_SPI_SCK_PORT, BSP_SPI_SCK_PIN, GPIO_PIN_SET)
#define SCK_CLR()  HAL_GPIO_WritePin(BSP_SPI_SCK_PORT, BSP_SPI_SCK_PIN, GPIO_PIN_RESET)
#define MOSI_SET() HAL_GPIO_WritePin(BSP_SPI_MOSI_PORT,BSP_SPI_MOSI_PIN,GPIO_PIN_SET)
#define MOSI_CLR() HAL_GPIO_WritePin(BSP_SPI_MOSI_PORT,BSP_SPI_MOSI_PIN,GPIO_PIN_RESET)
#define RES_SET()  HAL_GPIO_WritePin(BSP_SPI_RES_PORT, BSP_SPI_RES_PIN, GPIO_PIN_SET)
#define RES_CLR()  HAL_GPIO_WritePin(BSP_SPI_RES_PORT, BSP_SPI_RES_PIN, GPIO_PIN_RESET)

/* ---- 短延时 (~1us @168MHz) ---- */
static void spi_delay(void)
{
    /* 168MHz / 1M ≈ 168 周期, 约 1us */
    for (volatile uint8_t i = 0; i < 20; i++) { __NOP(); }
}

/* ================================================================
 * bsp_sw_spi_init — 初始化所有 SPI GPIO 为推挽输出
 * 初始状态: CS=高(不选中), DC=高(数据), SCK=低, MOSI=低, RES=高
 * ================================================================*/
void bsp_sw_spi_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    /* SCK (PE9) */
    GPIO_InitStruct.Pin = BSP_SPI_SCK_PIN;
    HAL_GPIO_Init(BSP_SPI_SCK_PORT, &GPIO_InitStruct);

    /* MOSI(PE10) */
    GPIO_InitStruct.Pin = BSP_SPI_MOSI_PIN;
    HAL_GPIO_Init(BSP_SPI_MOSI_PORT, &GPIO_InitStruct);

    /* RES (PE11) */
    GPIO_InitStruct.Pin = BSP_SPI_RES_PIN;
    HAL_GPIO_Init(BSP_SPI_RES_PORT, &GPIO_InitStruct);

    /* DC  (PE12) */
    GPIO_InitStruct.Pin = BSP_SPI_DC_PIN;
    HAL_GPIO_Init(BSP_SPI_DC_PORT, &GPIO_InitStruct);

    /* CS  (PE13) */
    GPIO_InitStruct.Pin = BSP_SPI_CS_PIN;
    HAL_GPIO_Init(BSP_SPI_CS_PORT, &GPIO_InitStruct);

    /* 初始状态 */
    CS_SET();
    DC_SET();
    SCK_CLR();
    MOSI_CLR();
    RES_SET();
}

/* ================================================================
 * CS / DC / RES 控制 (供上层 DEV 调用)
 * ================================================================*/
void bsp_sw_spi_cs_low(void)  { CS_CLR(); }
void bsp_sw_spi_cs_high(void) { CS_SET(); }
void bsp_sw_spi_dc_low(void)  { DC_CLR(); }
void bsp_sw_spi_dc_high(void) { DC_SET(); }
void bsp_sw_spi_res_low(void) { RES_CLR(); }
void bsp_sw_spi_res_high(void){ RES_SET(); }

/* ================================================================
 * bsp_sw_spi_write_byte — 发送 1 字节 (MSB First, Mode 0)
 *
 * 时序:
 *   1. SCK=低 (空闲)
 *   2. 在 SCK 上升沿之前建立 MOSI 数据
 *   3. SCK 上升沿 → 从机采样
 *   4. SCK 下降沿 → 准备下一位
 * ================================================================*/
void bsp_sw_spi_write_byte(uint8_t u8_data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        SCK_CLR();

        if (u8_data & 0x80)
            MOSI_SET();
        else
            MOSI_CLR();
        u8_data <<= 1;

        spi_delay();
        SCK_SET();
        spi_delay();
    }
    SCK_CLR();
}
