/**********************************************************
 * 文件名: dev_spi_oled.h
 * 日  期: 2026-07-31
 *
 * 功能说明:
 *   SSD1306 OLED 128x64 SPI 通用设备层驱动 (DEV 层)
 *   适用于 ELC-F4 开发板板载 SPI OLED (HS96L01W4S03)
 *   5 线制: DC/CS/SCK/MOSI/RES
 *   基于显存缓冲 (gram) 机制: 所有绘制写入显存后统一刷新
 *
 * 依赖:
 *   - BSP 层: bsp_sw_spi.h (软件 SPI 基础时序)
 *   - <stdint.h> 标准类型
 *
 * 架构分层: APP → DEV (本层) → BSP → HAL
 *
 * 与 dev_i2c_oled 的区别:
 *   仅底层通信协议不同 (SPI vs I2C), 上层 API 完全一致,
 *   上层应用无需修改代码即可切换两种驱动
 **********************************************************/

#ifndef __DEV_SPI_OLED_H
#define __DEV_SPI_OLED_H

#include <stdint.h>

/* ================================================================
 * 宏定义
 * ================================================================*/

#define DEV_OLED_WIDTH       128U
#define DEV_OLED_HEIGHT      64U
#define DEV_OLED_PAGES       8U

/* ================================================================
 * 全局显存 (8 页 × 128 列 = 1024 字节)
 * g_u8_oled_gram[page][col].bit(row) → 像素 (col, page*8 + row)
 * ================================================================*/
extern uint8_t g_u8_oled_gram[DEV_OLED_PAGES][DEV_OLED_WIDTH];

/* ================================================================
 * 基础控制 API
 * ================================================================*/

void dev_oled_init(void);
void dev_oled_clear(void);
void dev_oled_display_on(void);
void dev_oled_display_off(void);
void dev_oled_set_contrast(uint8_t val);   /* 0~255 */
void dev_oled_refresh_gram(void);

/* ================================================================
 * 图形绘制 API (坐标原点: 左上角, x 向右, y 向下)
 * ================================================================*/

void dev_oled_draw_point(uint8_t x, uint8_t y, uint8_t t);
void dev_oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void dev_oled_draw_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fill);
void dev_oled_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void dev_oled_draw_circle(uint8_t x0, uint8_t y0, uint8_t r);

/* ================================================================
 * 字符与数字显示 API
 *
 * 字号参数:
 *   size = 12 → F6x8  字模 (6×8  像素, 每字符 x 推进 6)
 *   size = 16 → F8x16 字模 (8×16 像素, 每字符 x 推进 8)
 *
 * 写入策略: 先清空字符占用的显存区域, 再写入新字模 (防重影)
 * ================================================================*/

void dev_oled_show_char(uint8_t x, uint8_t y, char chr, uint8_t size);
void dev_oled_show_string(uint8_t x, uint8_t y, const char *p_str);
void dev_oled_show_string_size(uint8_t x, uint8_t y, const char *p_str, uint8_t size);
void dev_oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);

#endif /* __DEV_SPI_OLED_H */
