#ifndef __APP_CLI_H
#define __APP_CLI_H

#include <stdint.h>

void APP_CLI_Init(void);
void APP_CLI_Task(void *arg);
void APP_CLI_RxPush(uint8_t ch);

#endif /* __APP_CLI_H */
