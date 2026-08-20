#ifndef __DEV_SD_H
#define __DEV_SD_H

#include "bsp_spi.h"

/* SD 卡类型 */
#define SD_TYPE_ERR  0x00
#define SD_TYPE_V2HC 0x06

/* SPI 模式命令 */
#define CMD0   0
#define CMD8   8
#define CMD17  17
#define CMD24  24
#define CMD41  41
#define CMD55  55
#define CMD58  58

uint8_t  DEV_SD_Init(void);
uint8_t  DEV_SD_ReadDisk(uint8_t *buf, uint32_t sector, uint32_t count);
uint8_t  DEV_SD_WriteDisk(const uint8_t *buf, uint32_t sector, uint32_t count);
uint32_t DEV_SD_GetSectorCount(void);

#endif
