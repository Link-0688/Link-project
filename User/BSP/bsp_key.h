#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "main.h"
#include <stdint.h>

typedef enum{
    KEY_1 = 0,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_NUM
}key_id_t;

void BSP_Key_Init(void);
uint8_t BSP_Key_Read(key_id_t id);

#endif /*__BSP_KEY_H*/
