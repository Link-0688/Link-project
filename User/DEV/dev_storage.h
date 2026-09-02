#ifndef __DEV_STORAGE_H
#define __DEV_STORAGE_H

#include <stdint.h>

/* CRC32 (标准多项式 0xEDB88320) */
uint32_t DEV_Storage_CRC32(const uint8_t *p_data, uint32_t len);

/*
 * 带校验写: 内容 = [magic 4B][crc32 4B][payload]
 * 先写临时文件再 rename 覆盖, 掉电时要么旧文件要么新文件, 不会出现半截损坏文件。
 * 返回 1 成功, 0 失败。
 */
uint8_t DEV_Storage_Write(const char *path, const uint8_t *p_data, uint32_t len);

/*
 * 带校验读: 校验 magic + crc, 全部通过才把 payload 拷到 out。
 * 返回 1 成功, 0 失败(文件不存在/损坏), 此时调用方应使用默认值。
 */
uint8_t DEV_Storage_Read(const char *path, uint8_t *p_out, uint32_t len);

#endif /* __DEV_STORAGE_H */
