#include "app_cli.h"
#include "main.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_event.h"
#include "app_log.h"
#include "dev_config.h"
#include "app_health.h"
#include <stdio.h>
#include <string.h>

#define APP_VERSION "1.5.0"
#define CLI_RX_BUF_SIZE 128
#define CLI_LINE_MAX    64

static volatile uint8_t s_rx_buf[CLI_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;
static uint8_t s_rx_char = 0;   /*单字节接收缓冲，仅ISR回调内访问*/

void APP_CLI_RxPush(uint8_t ch)
{
    uint16_t next = (uint16_t)((s_rx_head + 1) % CLI_RX_BUF_SIZE);
    if(next == s_rx_tail)   return; /*满则丢弃*/
    s_rx_buf[s_rx_head] = ch;
    s_rx_head = next;
}

static uint8_t rx_pop(uint8_t *ch)
{
    if(s_rx_tail == s_rx_head)  return 0;
    *ch = s_rx_buf[s_rx_tail];
    s_rx_tail = (uint16_t)((s_rx_tail + 1) % CLI_RX_BUF_SIZE);
    return 1;
}

void APP_CLI_Init(void)
{
    s_rx_head = 0;
    s_rx_tail = 0;
    HAL_UART_Receive_IT(&huart1,&s_rx_char,1);
}

/*HAL接收完成回调：接收一字符入环缓冲，再启动接收*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        APP_CLI_RxPush(s_rx_char);
        HAL_UART_Receive_IT(&huart1,&s_rx_char,1);
    }
}

/*错误回调：上电遗留数据触发 ORE/FE 会使接收中断停止，
 *此处清除错误标志并重启接收，否则后续输入无反应*/
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        /* ORE 时 HAL 已通过 UART_EndRxTransfer 复位 RxState，但 FE/NE 等非阻塞错误
         * 不会复位(仍为 BUSY_RX)，此时 HAL_UART_Receive_IT 会返回 HAL_BUSY 导致
         * 接收中断永久停摆。必须先把 RxState 强制复位为 READY 再重启。 */
        huart->RxState = HAL_UART_STATE_READY;
        HAL_UART_Receive_IT(&huart1,&s_rx_char,1);
    }
}

static void cmd_help(void)
{
    printf("\r\n=== CLI ===\r\n");
    printf("help          show this\r\n");
    printf("task          task list\r\n");
    printf("log           dump log\r\n");
    printf("config        show config\r\n");
    printf("ver           version\r\n");
    printf("crash         simulate crash (dump+reset)\r\n");
    printf("stuck <t>     suspend t=input/sys/ui\r\n");
    printf("fill          fill event queue\r\n");
    printf("reset         software reset\r\n");
}

static void cmd_task(void)
{
    char buf[512];
    vTaskList(buf);
    printf("\r\n%s",buf);
}

static void cmd_config(void)
{
    config_t c = DEV_Config_Get();
    printf("\r\nSENS=%u SIZE=%u BRIGHT=%u VOL=%u SLEEP=%us\r\n",
    c.sensitivity,c.cursor_size,c.brightness,c.volume,c.sleep_time);
}

static void cmd_stuck(const char *name)
{
    TaskHandle_t h = xTaskGetHandle(name);
    if(h == NULL)
    {
        printf("no task %s\r\n",name);
        return;
    }
    vTaskSuspend(h);

    printf("suspended %s\r\n",name);
}

static void cmd_fill(void)
{
    uint32_t n = 0;
    while(APP_Event_Send(EVT_KEY_PRESS,0))  n++;
    printf("filled %lu events\r\n",(unsigned long)n);
}

static void exec_line(char *line)
{
    size_t l = strlen(line);
    while(l > 0 && (line[l-1] == '\r' || line[l-1] == '\n' || line[l-1] == ' '))
    {
        line[--l] = '\0';
    }
    if(strcmp(line,"help") == 0) cmd_help();
    else if(strcmp(line,"task") == 0) cmd_task();
    else if(strcmp(line, "log") == 0)        APP_Log_Dump();
    else if(strcmp(line, "config") == 0)     cmd_config();
    else if(strcmp(line, "ver") == 0)        printf("\r\n%s\r\n", APP_VERSION);
    else if(strcmp(line, "crash") == 0)      APP_Health_CrashDump(CRASH_HARDFAULT);
    else if(strncmp(line, "stuck ", 6) == 0) cmd_stuck(line + 6);
    else if(strcmp(line, "fill") == 0)       cmd_fill();
    else if(strcmp(line, "reset") == 0)      NVIC_SystemReset();
    else if(strlen(line) > 0)                printf("\r\nunknown: %s\r\n", line);
}

void APP_CLI_Task(void *arg)
{
    (void)arg;
    char line[CLI_LINE_MAX];
    uint8_t idx = 0;
    printf("\r\n[CLI]ready,type'help'\r\n");
    for(;;)
    {
        uint8_t ch;
        if(!rx_pop(&ch))
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if(ch == '\r' || ch == '\n')
        {
            printf("\r\n");
            line[idx] = '\0';
            if(idx > 0) exec_line(line);
            idx = 0;
        }
        else if(ch == '\b' || ch == 0x7F)
        {
            if(idx > 0)
            {
                idx--;
                printf("\b \b");
            }
        }
        else
        {
            if(idx < CLI_LINE_MAX - 1)
            {
                line[idx++] = (char)ch;
                printf("%c",ch);
            }
        }
    }
}
