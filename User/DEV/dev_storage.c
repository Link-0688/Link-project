#include "dev_storage.h"
#include "ff.h"
#include <string.h>

#define STORAGE_MAGIC   0x53544F52UL    /*STOR*/
#define STORAGE_TMP_NAME    "~STOR.TMP"

static uint32_t s_crc_table[256];
static uint8_t  s_crc_ready = 0;

/*生成CRC32查找表*/
static void crc_make_table(void)
{
    for(uint32_t i = 0; i < 256; i++)
    {
        uint32_t c = i;
        for(uint8_t k = 0; k < 8; k++)
        {
            c = (c & 1) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
        }
        s_crc_table[i] = c;
    }
    s_crc_ready = 1;
}

uint32_t DEV_Storage_CRC32(const uint8_t *p_data,uint32_t len)
{
    if(!s_crc_ready)    crc_make_table();
    uint32_t crc = 0xFFFFFFFF;
    for(uint32_t i = 0; i < len; i++)
    {
        crc = s_crc_table[(crc ^ p_data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFUL;
}

uint8_t DEV_Storage_Write(const char *path, const uint8_t *p_data, uint32_t len)
{
    FIL file;
    UINT bw = 0;
    uint32_t magic = STORAGE_MAGIC;
    uint32_t crc = DEV_Storage_CRC32(p_data,len);
    /*写临时文件*/
    if(f_open(&file,STORAGE_TMP_NAME,FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) return 0;
    if(f_write(&file,&magic,4,&bw) != FR_OK || bw != 4) {f_close(&file); return 0;}
    if(f_write(&file,&crc,4,&bw) != FR_OK || bw != 4) {f_close(&file); return 0;}
    if(f_write(&file,p_data,len,&bw) != FR_OK || bw != len) {f_close(&file); return 0;}
    f_close(&file);
    /*原子替换：删旧 + rename临时文件*/
    f_unlink(path);
    if(f_rename(STORAGE_TMP_NAME,path) != FR_OK)    return 0;
    return 1;
}

uint8_t DEV_Storage_Read(const char *path, uint8_t *p_out, uint32_t len)
{
    FIL file;
    UINT br = 0;
    uint32_t magic = 0;
    uint32_t crc_file = 0;
    uint32_t crc_calc;

    if(f_open(&file,path,FA_READ) != FR_OK) return 0;
    if(f_read(&file,&magic,4,&br) != FR_OK || br != 4) {f_close(&file); return 0;}
    if(f_read(&file,&crc_file,4,&br) != FR_OK || br != 4) {f_close(&file); return 0;}
    if(f_read(&file,p_out,len,&br) != FR_OK || br != len) {f_close(&file); return 0;}
    f_close(&file);

    if(magic != STORAGE_MAGIC)  return 0;   /*魔数错->非本格式/损坏*/
    crc_calc = DEV_Storage_CRC32(p_out,len);
    if(crc_calc != crc_file)    return 0;   /*CRC错->数据损坏*/
    return 1;
}
