#ifndef __BSP_IWDG_H
#define __BSP_IWDG_H

void BSP_IWDG_Init(void);   /* 启动独立看门狗, LSI, 约 4 秒超时 */
void BSP_IWDG_Feed(void);   /* 喂狗 */

#endif /* __BSP_IWDG_H */
