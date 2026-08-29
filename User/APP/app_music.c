#include "app_music.h"
#include "app_ui_model.h"
#include "dev_spi_oled.h"
#include "dev_music.h"
#include "dev_config.h"
#include "bsp_key.h"
#include <stdio.h>
#include "tim.h"

/*曲目数据*/
static const Note_t s_song_star[] = {
    {NOTE_C4,400},{NOTE_C4,400},{NOTE_G4,400},{NOTE_G4,400},
    {NOTE_A4,400},{NOTE_A4,400},{NOTE_G4,800},
    {NOTE_F4,400},{NOTE_F4,400},{NOTE_E4,400},{NOTE_E4,400},
    {NOTE_D4,400},{NOTE_D4,400},{NOTE_C4,800},
    {NOTE_0,0}
};

static const Note_t s_song_tiger[] = {
    {NOTE_C4,400},{NOTE_D4,400},{NOTE_E4,400},{NOTE_C4,400},
    {NOTE_C4,400},{NOTE_D4,400},{NOTE_E4,400},{NOTE_C4,400},
    {NOTE_E4,400},{NOTE_F4,400},{NOTE_G4,800},
    {NOTE_E4,400},{NOTE_F4,400},{NOTE_G4,800},
    {NOTE_G4,200},{NOTE_A4,200},{NOTE_G4,200},{NOTE_F4,200},{NOTE_E4,400},{NOTE_C4,400},
    {NOTE_G4,200},{NOTE_A4,200},{NOTE_G4,200},{NOTE_F4,200},{NOTE_E4,400},{NOTE_C4,400},
    {NOTE_C4,400},{NOTE_G4,400},{NOTE_C4,800},
    {NOTE_0,0}
};

static const Note_t s_song_mary[] = {
    {NOTE_E4,400},{NOTE_D4,400},{NOTE_C4,400},{NOTE_D4,400},
    {NOTE_E4,400},{NOTE_E4,400},{NOTE_E4,800},
    {NOTE_D4,400},{NOTE_D4,400},{NOTE_D4,800},
    {NOTE_E4,400},{NOTE_G4,400},{NOTE_G4,800},
    {NOTE_E4,400},{NOTE_D4,400},{NOTE_C4,400},{NOTE_D4,400},
    {NOTE_E4,400},{NOTE_E4,400},{NOTE_E4,400},{NOTE_E4,400},
    {NOTE_D4,400},{NOTE_D4,400},{NOTE_E4,400},{NOTE_D4,400},{NOTE_C4,800},
    {NOTE_0,0}
};

typedef struct{
    const Note_t *notes;
    const char   *name;
}song_t;

static const song_t s_songs[] = {
    {s_song_star, "LITTLE STAR"},
    {s_song_tiger,"TWO TIGERS"},
    {s_song_mary,"MARY LAMB"},
};

#define SONG_NUM (sizeof(s_songs) / sizeof(s_songs[0]))

static uint8_t s_current;
static uint8_t s_playing;

/*播放当前曲目(压入当前缓冲)*/
static void play_song(uint8_t index)
{
    DEV_Music_Stop();
    const Note_t *p = s_songs[index].notes;
    while(p->duration_ms != 0)
    {
        DEV_Music_PushNote(*p);
        p++;
    }
    s_playing = 1;
}

void APP_Music_Init(void)
{
    s_current = 0;
    s_playing = 0;

    BSP_Buzzer_Init();
    BSP_Buzzer_SetVolume(DEV_Config_Get().volume);
    DEV_Music_Init();

    HAL_TIM_Base_Start_IT(&htim6);
}

void APP_Music_Exit(void)
{
    DEV_Music_Stop();
    HAL_TIM_Base_Stop_IT(&htim6);
    s_playing = 0;
}

void APP_Music_HandleEvent(input_event_t *p_evt)
{
    APP_UIModel_Lock();
    if(p_evt->type == EVT_ENC_RIGHT)
    {
        s_current = (uint8_t)((s_current + 1) % SONG_NUM);
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_ENC_LEFT)
    {
        s_current = (uint8_t)((s_current + SONG_NUM - 1) % SONG_NUM);
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_3)
    {
        play_song(s_current);
        g_ui_model.dirty = 1;
    }
    else if(p_evt->type == EVT_KEY_PRESS && p_evt->param == KEY_4)
    {
        DEV_Music_Stop();
        s_playing = 0;
        g_ui_model.dirty = 1;
    }
    APP_UIModel_Unlock();
}

void APP_Music_Render(void)
{
    s_playing = DEV_Music_IsBusy();

    for(uint8_t i = 0; i < SONG_NUM; i++)
    {
        uint8_t y = (uint8_t)(0 + i * 16);
        if(i == s_current)
        {
            dev_oled_show_string(0,y,">");
        }
        dev_oled_show_string(8,y,s_songs[i].name);
    }
    if(s_playing)
    {
        dev_oled_show_string(0,48,"PLAYING...");
    }
    else
    {
        dev_oled_show_string(0,48,"STOPPED");
    }
}
