#ifndef __DEV_MUSIC_H
#define __DEV_MUSIC_H

#include "main.h"
#include "bsp_buzzer.h"

/* 音阶定义 (Hz) */
#define NOTE_0   0
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494

/* 音符数据结构 */
typedef struct {
    uint16_t frequency;   // 频率 (Hz)
    uint16_t duration_ms; // 持续时间 (ms)
} Note_t;

/* 环形缓冲区结构体 */
#define MUSIC_BUFFER_SIZE 64

typedef struct {
    Note_t buffer[MUSIC_BUFFER_SIZE];
    volatile uint16_t head; // 读指针 (中断使用)
    volatile uint16_t tail; // 写指针 (应用层使用)
    volatile uint16_t count;// 当前缓冲区中的音符数量
} MusicRingBuffer_t;

/* 接口函数 */
void DEV_Music_Init(void);
uint8_t DEV_Music_PushNote(Note_t note);
void DEV_Music_TIM6_IRQHandler(void); // 定时器中断服务调用
void DEV_Music_PlayLittleStar_Async(void);

#endif /* __DEV_MUSIC_H */
