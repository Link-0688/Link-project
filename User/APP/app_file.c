#include "app_file.h"
#include "app_ui_model.h"
#include "dev_spi_oled.h"
#include "bsp_key.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>
#include "app_sys.h"

#define FILE_NAME_MAX       24
#define FILE_TYPE_MAX       8
#define FILE_LIST_MAX       16
#define FILE_VISIBLE_ROWS   3   /*屏幕可显示的文件行数(y=16/32/48)*/

typedef struct{
    char    name[FILE_NAME_MAX];
    char    type[FILE_TYPE_MAX];
    uint32_t size;
    uint8_t status; /*0 = 正常*/
}file_entry_t;

static file_entry_t s_files[FILE_LIST_MAX];
static uint16_t     s_file_count;
static uint8_t      s_cursor;
static uint8_t      s_scroll;   /*列表顶部偏移，让光标保持在可见窗口内*/
static uint8_t      s_mode;     /*0 = 列表，1 = 查看，2 = 删除确认 3 = 新建确认*/
static char s_new_name[FILE_NAME_MAX];

static void detect_type(const char *name,char *type, uint8_t len)
{
    const char *dot = strrchr(name,'.');
    if(dot == NULL) {strncpy(type,"OTHER",len); return;}
    if(strcmp(dot,".TXT") == 0 || strcmp(dot,".txt") == 0) strncpy(type,"TXT",len);
    else if(strcmp(dot,".BIN") == 0 || strcmp(dot,".bin") == 0) strncpy(type,"BIN",len);
    else if(strcmp(dot,".PIC") == 0 || strcmp(dot,".pic") == 0) strncpy(type,"PIC",len);
    else strncpy(type,"OTHER",len);
}

static uint8_t scan_files(void)
{
    DIR dir;
    FRESULT res = f_opendir(&dir,"/");
    if(res != FR_OK)    return 0;
    s_file_count = 0;
    FILINFO fno;
    for(;;)
    {
        res = f_readdir(&dir,&fno);
        if(res != FR_OK || fno.fname[0] == 0)   break;
        if(fno.fattrib & AM_DIR)    continue;   /*跳过目录*/
        if(s_file_count >= FILE_LIST_MAX)   break;
        strncpy(s_files[s_file_count].name, fno.fname, FILE_NAME_MAX - 1);
        s_files[s_file_count].name[FILE_NAME_MAX - 1] = '\0';
        detect_type(fno.fname, s_files[s_file_count].type, FILE_TYPE_MAX);
        s_files[s_file_count].size = fno.fsize;
        s_files[s_file_count].status = 0;
        s_file_count++;
    }
    f_closedir(&dir);
    return 1;
}

/*重名检测：文件名已存在返回1*/
static uint8_t name_exists(const char *name)
{
    for(uint16_t i = 0; i < s_file_count; i++)
    {
        if(strcmp(s_files[i].name,name) == 0)   return 1;
    }
    return 0;
}

/*生成下一个不重名的自动文件名FILE001.TXT*/
static void gen_new_name(char *out, uint16_t len)
{
    for(uint16_t i = 1; i < 1000; i++)
    {
        snprintf(out,len,"FILE%03u.TXT",(unsigned)i);
        if(!name_exists(out))   return;
    }
    out[0] = '\0';
}

static void create_new_file(void)
{
    FIL file;
    if(f_open(&file,s_new_name,FA_CREATE_NEW | FA_WRITE) == FR_OK)
    {
        f_close(&file);
    }
    scan_files();
}

void APP_File_Init(void)
{
    s_cursor = 0;
    s_scroll = 0;
    s_mode   = 0;
    s_new_name[0] = '\0';
    if (!APP_Sys_IsSdReady()) 
    {
        s_file_count = 0;
        return;
    }
    scan_files();
}

void APP_File_Exit(void)
{
    /*无需清理*/
}

/*让光标保持在可见窗口内*/
static void scroll_to_cursor(void)
{
    if(s_cursor < s_scroll)
    {
        s_scroll = s_cursor;
    }
    else if(s_cursor >= s_scroll + FILE_VISIBLE_ROWS)
    {
        s_scroll = (uint8_t)(s_cursor - FILE_VISIBLE_ROWS + 1);
    }
}

/*删除当前光标文件*/
static void delete_current(void)
{
    if(s_cursor >= s_file_count)    return;
    f_unlink(s_files[s_cursor].name);
    scan_files();
    if(s_cursor >= s_file_count)    s_cursor = 0;
    scroll_to_cursor();
}

void APP_File_HandleEvent(input_event_t *p_evt)
{
    APP_UIModel_Lock();

    /* 确认态(删除/新建)优先处理 */
    if(s_mode == 2 || s_mode == 3)
    {
        if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_1)
        {
            if(s_mode == 2) delete_current();
            else            create_new_file();
            s_mode = 0;
            g_ui_model.dirty = 1;
        }
        else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_2)
        {
            s_mode = 0;
            g_ui_model.dirty = 1;
        }
        APP_UIModel_Unlock();
        return;
    }

    if(s_mode == 0)
    {
        if(p_evt->type == EVT_ENC_RIGHT)
        {
            if(s_cursor < s_file_count - 1) s_cursor++;
            scroll_to_cursor();
            g_ui_model.dirty = 1;
        }
        else if(p_evt->type == EVT_ENC_LEFT)
        {
            if(s_cursor > 0) s_cursor--;
            scroll_to_cursor();
            g_ui_model.dirty = 1;
        }
        else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_1)
        {
            gen_new_name(s_new_name, FILE_NAME_MAX);   /* 新建 */
            s_mode = 3;
            g_ui_model.dirty = 1;
        }
        else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_3)
        {
            if(s_file_count > 0) { s_mode = 1; g_ui_model.dirty = 1; }  /* 查看 */
        }
        else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_4)
        {
            if(s_file_count > 0) { s_mode = 2; g_ui_model.dirty = 1; }  /* 删除 */
        }
    }
    else if(s_mode == 1)
    {
        if(p_evt->type == EVT_KEY_PRESS)    { s_mode = 0; g_ui_model.dirty = 1; }
    }

    APP_UIModel_Unlock();
}

void APP_File_Render(void)
{
    char buf[32];
    if(!APP_Sys_IsSdReady())
    {
        dev_oled_show_string(0,0,"FILE");
        dev_oled_show_string(20,24,"NO SD CARD");
        return;
    }

    /*新建确认*/
    if(s_mode == 3)
    {
        dev_oled_show_string(0,0,"NEW FILE?");
        dev_oled_show_string(0,20,s_new_name);
        dev_oled_show_string(0,40,"1 = YES 2= NO");
        return;
    }
    /*删除确认*/
    if(s_mode  == 2)
    {
        dev_oled_show_string(0,0,"DELETE?");
        dev_oled_show_string(0,20,s_files[s_cursor].name);
        dev_oled_show_string(0,40,"1 = YES 2 = NO");
        return;
    }

    if(s_file_count == 0)
    {
        dev_oled_show_string(0,0,"FILE");
        dev_oled_show_string(20,24,"EMPTY");
        dev_oled_show_string(0,48,"KEY1 = NEW");
        return;
    }
    /*列表模式*/
    if(s_mode == 0)
    {
        snprintf(buf,sizeof(buf),"FILES:%d",s_file_count);
        dev_oled_show_string(0,0,buf);
        for(uint8_t i = 0; i < FILE_VISIBLE_ROWS; i++)
        {
            uint16_t idx = (uint16_t)(s_scroll + i);
            if(idx >= s_file_count) break;
            uint8_t y = (uint8_t)(16 + i * 16);
            if(idx == s_cursor)
            {
                dev_oled_show_string(0,y,">");
            }
            dev_oled_show_string(6,y,s_files[idx].name);
        }
    }
    /*查看模式*/
    else if(s_mode == 1)
    {
        dev_oled_show_string(0,0,"FILE INFO");
        dev_oled_show_string(0,16,s_files[s_cursor].name);
        snprintf(buf,sizeof(buf),"SIZE:%lu B",(unsigned long)s_files[s_cursor].size);
        dev_oled_show_string(0,32,buf);
        dev_oled_show_string(0,48,"ANY KEY BACK");
    }
}
