#include "dev_sd.h"
#include <stdio.h>

/* 初始化完成后切换到高速 SPI，加速 FATFS 读写（落盘阻塞是"1秒返回"/心跳超时的根因） */
static void SD_EnableFastSpi(void)
{
    /* BR=001 -> APB1(42MHz)/4 = 10.5MHz */
    MODIFY_REG(hspi2.Instance->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_4);
}

/* 发送命令帧 + 等 R1（含完整 CS↑→dummy→CS↓ 序列） */
static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t r1;
    uint16_t retry = 0;

    SPI2_CS_HIGH();
    BSP_SPI2_ReadWriteByte(0xFF);   /* 8 个时钟，CS=HIGH */
    SPI2_CS_LOW();

    BSP_SPI2_ReadWriteByte(cmd | 0x40);
    BSP_SPI2_ReadWriteByte((uint8_t)(arg >> 24));
    BSP_SPI2_ReadWriteByte((uint8_t)(arg >> 16));
    BSP_SPI2_ReadWriteByte((uint8_t)(arg >> 8));
    BSP_SPI2_ReadWriteByte((uint8_t)(arg));
    BSP_SPI2_ReadWriteByte(crc);

    do {
        r1 = BSP_SPI2_ReadWriteByte(0xFF);
        if (++retry > 200) break;
    } while (r1 & 0x80);

    return r1;
}

uint8_t DEV_SD_Init(void)
{
    uint8_t  r1, buf[4];
    uint16_t retry;

    printf("[SD] Init @164kHz (SPI2)\r\n");

    /* 1. ≥74 Dummy Clock，唤醒卡到 SPI 模式 */
    SPI2_CS_HIGH();
    for (int i = 0; i < 10; i++)
        BSP_SPI2_ReadWriteByte(0xFF);

    /* 2. CMD0: 软件复位 → 进入 Idle */
    printf("[SD] CMD0...");
    for (retry = 0; retry < 10; retry++) {
        r1 = SD_SendCmd(CMD0, 0, 0x95);
        if (r1 == 0x01) break;
    }
    printf("R1=0x%02X\r\n", r1);
    if (r1 != 0x01) { printf("[SD] FAIL: CMD0\r\n"); return 1; }

    /* 3. CMD8: 接口条件检查（电压 + 版本） */
    printf("[SD] CMD8...");
    r1 = SD_SendCmd(CMD8, 0x1AA, 0x87);
    printf("R1=0x%02X", r1);
    if (r1 == 0x01) {
        for (int i = 0; i < 4; i++) buf[i] = BSP_SPI2_ReadWriteByte(0xFF);
        printf(" R7=%02X%02X%02X%02X\r\n", buf[0], buf[1], buf[2], buf[3]);
    } else {
        printf(" (old card?)\r\n");
        SPI2_CS_HIGH();
        return 1;
    }

    /* 4. ACMD41: 等待卡退出 Idle
     *    每次 CMD55→CMD41 为一轮，CS 由 SD_SendCmd 自然管理 */
    printf("[SD] ACMD41...");
    for (retry = 0; retry < 3000; retry++) {
        r1 = SD_SendCmd(CMD55, 0, 0xFF);
        if (r1 > 1) continue;

        r1 = SD_SendCmd(CMD41, 0x40000000, 0xFF);
        /* 消费 ACMD41 的 R3 响应剩余 4 字节 OCR */
        BSP_SPI2_ReadWriteByte(0xFF);
        BSP_SPI2_ReadWriteByte(0xFF);
        BSP_SPI2_ReadWriteByte(0xFF);
        BSP_SPI2_ReadWriteByte(0xFF);

        if (r1 == 0x00) break;
    }
    printf("R1=0x%02X retry=%d\r\n", r1, retry);
    if (r1 != 0x00) { printf("[SD] FAIL: ACMD41 timeout\r\n"); SPI2_CS_HIGH(); return 1; }

    /* 5. CMD58 读取 OCR，确认卡已上电完成 */
    printf("[SD] CMD58...");
    r1 = SD_SendCmd(CMD58, 0, 0xFF);
    for (int i = 0; i < 4; i++) buf[i] = BSP_SPI2_ReadWriteByte(0xFF);
    printf("R1=0x%02X OCR=%02X%02X%02X%02X\r\n", r1, buf[0], buf[1], buf[2], buf[3]);

    if (r1 != 0x00) {
        printf("[SD] FAIL: Card stuck in idle, OCR bit31=%d\r\n",
               (buf[0] & 0x80) ? 1 : 0);
        SPI2_CS_HIGH();
        return 1;
    }

    SPI2_CS_HIGH();
    SD_EnableFastSpi();
    printf("[SD] Init OK (%s)\r\n", (buf[0] & 0x40) ? "SDHC" : "SDSC");
    return 0;
}

uint8_t DEV_SD_ReadDisk(uint8_t *buf, uint32_t sector, uint32_t count)
{
    if (count != 1) return 1;

    uint8_t r1 = SD_SendCmd(CMD17, sector, 0xFF);
    if (r1 != 0x00) {
        printf("[SD] RD s%lu R1=0x%02X ERR\r\n", sector, r1);
        SPI2_CS_HIGH();
        return 1;
    }

    /* 等待数据令牌 0xFE */
    uint32_t retry = 0;
    uint8_t  token;
    do {
        token = BSP_SPI2_ReadWriteByte(0xFF);
        if (++retry > 200000) {
            printf("[SD] RD s%lu TOKEN Timeout(last=0x%02X)\r\n", sector, token);
            SPI2_CS_HIGH();
            return 1;
        }
    } while (token != 0xFE);

    for (int i = 0; i < 512; i++)
        buf[i] = BSP_SPI2_ReadWriteByte(0xFF);
    BSP_SPI2_ReadWriteByte(0xFF); /* CRC 高字节 */
    BSP_SPI2_ReadWriteByte(0xFF); /* CRC 低字节 */

    SPI2_CS_HIGH();
    return 0;
}

uint8_t DEV_SD_WriteDisk(const uint8_t *buf, uint32_t sector, uint32_t count)
{
    if (count != 1) return 1;

    if (SD_SendCmd(CMD24, sector, 0xFF) != 0x00) { SPI2_CS_HIGH(); return 1; }

    BSP_SPI2_ReadWriteByte(0xFF);
    BSP_SPI2_ReadWriteByte(0xFE);

    for (int i = 0; i < 512; i++)
        BSP_SPI2_ReadWriteByte(buf[i]);
    BSP_SPI2_ReadWriteByte(0xFF);
    BSP_SPI2_ReadWriteByte(0xFF);

    uint8_t resp = BSP_SPI2_ReadWriteByte(0xFF);
    if ((resp & 0x1F) != 0x05) { SPI2_CS_HIGH(); return 1; }

    uint16_t timeout = 0;
    while (BSP_SPI2_ReadWriteByte(0xFF) != 0xFF)
        if (++timeout > 60000) { SPI2_CS_HIGH(); return 1; }

    SPI2_CS_HIGH();
    return 0;
}

uint32_t DEV_SD_GetSectorCount(void)
{
    uint8_t csd[16];
    uint8_t r1 = SD_SendCmd(CMD9, 0, 0xFF);
    if (r1 != 0x00) { SPI2_CS_HIGH(); return 0; }

    /* 等待数据令牌 0xFE */
    uint32_t retry = 0;
    uint8_t  token;
    do {
        token = BSP_SPI2_ReadWriteByte(0xFF);
        if (++retry > 200000) { SPI2_CS_HIGH(); return 0; }
    } while (token != 0xFE);

    for (int i = 0; i < 16; i++)
        csd[i] = BSP_SPI2_ReadWriteByte(0xFF);
    BSP_SPI2_ReadWriteByte(0xFF);   /* CRC 高字节 */
    BSP_SPI2_ReadWriteByte(0xFF);   /* CRC 低字节 */
    SPI2_CS_HIGH();

    /* CSD v2.0 (SDHC/SDXC)：C_SIZE 为 22 位，容量(块) = (C_SIZE+1)*1024 */
    uint8_t csd_ver = (uint8_t)((csd[0] >> 6) & 0x03);
    if (csd_ver == 1)
    {
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) |
                          ((uint32_t)csd[8] << 8) |
                          (uint32_t)csd[9];
        return (c_size + 1) * 1024;
    }
    return 0;   /* CSD v1.0(SDSC) 或未知：CMD8 已拒绝老卡，返回 0 */
}
