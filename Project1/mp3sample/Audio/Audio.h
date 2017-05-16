/****************************************Copyright (c)**************************************************                         
**
**                                 http://www.powermcu.com
**
**--------------File Info-------------------------------------------------------------------------------
** File name:			Audio.h
** Descriptions:		None
**
**------------------------------------------------------------------------------------------------------
** Created by:			AVRman
** Created date:		2010-11-18
** Version:				1.0
** Descriptions:		The original version
**
**------------------------------------------------------------------------------------------------------
** Modified by:			
** Modified date:	
** Version:
** Descriptions:		
********************************************************************************************************/
#ifndef __AUDIO_H
#define __AUDIO_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "mp3common.h"
#include "MP3Header.h"
#include <string.h>
#include <stdio.h>

/* Private define ------------------------------------------------------------*/
#define READBUF_SIZE   ( 5 * 1024 )      /* Value must min be 2xMAINBUF_SIZE = 2x1940 = 3880bytes */

/* Private variables ---------------------------------------------------------*/
/* file system*/
extern FATFS    fs;                      /* Work area (file system object) for logical drive */
extern FIL      mp3FileObject;           /* file objects */
extern FRESULT  res;                     /* FatFs function common result code	*/
extern UINT     n_Read;                  /* File R/W count */
extern uint8_t  readBuf[READBUF_SIZE];   /* Read buffer where data from SD card is read to */
extern uint32_t MP3_Data_Index;			 /* MP3已播放大小 */
extern volatile uint32_t outOfData; 	 

/* Private typedef -----------------------------------------------------------*/
typedef enum { FULL=0,LOW_EMPTY=1,HIGH_EMPTY=2 }                AUDIO_PlaybackBuffer_Status ;
typedef enum { MONO, STEREO}                                    AUDIO_Format_enum;
typedef enum { NO_SOUND, IS_PLAYING }                           AUDIO_Playback_status_enum;
typedef enum { LG_8_BITS=8, LG_16_BITS=16}  AUDIO_Length_enum;

/* Type to hold data for the Flying text */
typedef struct
{
    char text[64];  /* Maximum 64 chars */
    u8 text_length; /* Length of the text */
    u8 curr_pos;    /* Current pointer for first char to write	*/
    u8 x;           /* Position to write first char */
    u8 y;           /* Position to write first char */
    u8 disp_length; /* How many chars to show on screen */
    u16 time;       /* How fast to roll the text */
} tflying_text;

/* Type used to read out ID3 Tag, this type fits to the ID3V1 Format */
typedef struct
{
    char tag[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    uint8_t genre;
} tID3V3;





/* Private function prototypes -----------------------------------------------*/
void AUDIO_Playback_Stop(void);
void PlayAudioFile(FIL *FileObject,const char *path);
///void PlayAudioFile(FIL *FileObject,void *path);
const char* MP3_GetStrResult(int err);
void AUDIO_Play( s8 * buffer, int size ); 
int FillBuffer(FIL *FileObject , uint8_t start)  ;
void DMA_Configuration( s8 * addr, int size);
void I2S_Configuration(uint32_t I2S_AudioFreq);
AUDIO_PlaybackBuffer_Status AUDIO_PlaybackBuffer_GetStatus(AUDIO_PlaybackBuffer_Status value);
void AUDIO_Init_audio_mode(AUDIO_Length_enum length, uint16_t frequency, AUDIO_Format_enum format);

void MP3_NVIC_Config4TIM6(void);
void MP3_TIM6_Configuration(uint32_t freq);
void TIM6_IRQHandler(void);
void dac_outMP3(void);
volatile void DAC_Config2(void);
void AudioStart(void);
#endif 
/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
