#include "bsp_iwdg.h"
#include "main.h"

/*
 * 独立看门狗(IWDG) 由 LSI(约32kHz) 驱动, 一旦启动无法关闭, 只能复位清除。
 * 超时计算: LSI=32kHz, 预分频 PR=6 -> 256 分频 -> 32k/256 = 125Hz;
 *           重载值 RLR=500 -> 500/125 = 4 秒。
 * 喂狗周期在 TaskHealth 里为 1 秒, 留 4 倍余量。
 */
void BSP_IWDG_Init(void)
{
    /* 调试时暂停 IWDG, 避免断点/单步触发复位 (仅调试器连接时生效) */
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;

    /* 使能 LSI 并等待就绪 */
    RCC->CSR |= RCC_CSR_LSION;
    while((RCC->CSR & RCC_CSR_LSIRDY) == 0) { }

    IWDG->KR  = 0x5555;     /* 解锁 PR/RLR */
    IWDG->PR  = 6;          /* 预分频 256 (PR[2:0]=110) */
    IWDG->RLR = 500;        /* 重载值 -> 约 4 秒超时 */
    IWDG->KR  = 0xCCCC;     /* 启动 IWDG */
    IWDG->KR  = 0xAAAA;     /* 启动后立即喂一次 */
}

void BSP_IWDG_Feed(void)
{
    IWDG->KR = 0xAAAA;
}
