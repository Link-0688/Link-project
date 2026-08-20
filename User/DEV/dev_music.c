#include "dev_music.h"
#include <stdio.h>

static MusicRingBuffer_t music_rb;
static uint16_t note_elapsed_ms = 0;
static Note_t current_note = {0, 0};

/**
  * @brief 初始化音乐环形缓冲区与状态
  */
void DEV_Music_Init(void)
{
    music_rb.head = 0;
    music_rb.tail = 0;
    music_rb.count = 0;
    note_elapsed_ms = 0;
    current_note.frequency = 0;
    current_note.duration_ms = 0;
}

/**
  * @brief 向环形缓冲区压入一个音符 (生产者)
  */
uint8_t DEV_Music_PushNote(Note_t note)
{
    if (music_rb.count >= MUSIC_BUFFER_SIZE)
    {
        return 0;
    }
    music_rb.buffer[music_rb.tail] = note;
    music_rb.tail = (music_rb.tail + 1) % MUSIC_BUFFER_SIZE;

    __disable_irq();
    music_rb.count++;
    __enable_irq();

    return 1;
}

/**
  * @brief TIM6 中断回调逻辑 (每 10ms 调用一次, 消费者)
  */
void DEV_Music_TIM6_IRQHandler(void)
{
    /* 当前无音符播放 → 从缓冲区取下一个 */
    if (current_note.duration_ms == 0)
    {
        if (music_rb.count > 0)
        {
            current_note = music_rb.buffer[music_rb.head];
            music_rb.head = (music_rb.head + 1) % MUSIC_BUFFER_SIZE;
            __disable_irq();
            music_rb.count--;
            __enable_irq();
            note_elapsed_ms = 0;

            if (current_note.frequency != NOTE_0)
            {
                BSP_Buzzer_SetFrequency(current_note.frequency);
            }
            else
            {
                BSP_Buzzer_Stop();
            }
        }
        else
        {
            BSP_Buzzer_Stop();
        }
        return;
    }

    /* 当前音符播放中 → 累计时间 */
    if (current_note.duration_ms > 0)
    {
        note_elapsed_ms += 10;

        if (note_elapsed_ms >= current_note.duration_ms)
        {
            BSP_Buzzer_Stop();
            current_note.duration_ms = 0;
        }
    }
}

/**
  * @brief 异步将《小星星》压入缓冲区
  */
void DEV_Music_PlayLittleStar_Async(void)
{
    static const Note_t LittleStar[] =
    {
        {NOTE_C4, 300}, {NOTE_C4, 300}, {NOTE_G4, 300}, {NOTE_G4, 300},
        {NOTE_A4, 300}, {NOTE_A4, 300}, {NOTE_G4, 600},
        {NOTE_F4, 300}, {NOTE_F4, 300}, {NOTE_E4, 300}, {NOTE_E4, 300},
        {NOTE_D4, 300}, {NOTE_D4, 300}, {NOTE_C4, 600},
        {NOTE_0, 0}
    };

    uint16_t i = 0;
    while (LittleStar[i].duration_ms != 0)
    {
        DEV_Music_PushNote(LittleStar[i]);
        i++;
    }
    printf("[ASYNC MUSIC] Little Star added to ring buffer!\r\n");
}
