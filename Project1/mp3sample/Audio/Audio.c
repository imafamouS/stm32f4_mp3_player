/****************************************Copyright (c)**************************************************                         
**
**                                 http://www.powermcu.com
**
**--------------File Info-------------------------------------------------------------------------------
** File name:			Audio.C
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

/* Includes ------------------------------------------------------------------*/
////#include "main.h"
#include "Audio.h"

BYTE Buff[4096] __attribute__ ((aligned(4)));
/////// extern ///from "waveplay"
extern __IO uint8_t Command_index;		///use! ///usbh_usr.c	  懡暘 0:play  1:stop 
/// extern FATFS fatfs;
/// extern FIL file;
/// extern FIL fileR;
/// extern DIR dir;
/// extern FILINFO fno;
extern __IO uint8_t AudioPlayStart;
extern uint16_t *CurrentPos;		// NOT USE ///stm32f4_discovery_audio_codec.c
extern USB_OTG_CORE_HANDLE USB_OTG_Core;	// USE  ///main.c
extern uint8_t WaveRecStatus;		// not USE 	///waverecorder.c	 
extern __IO uint32_t WaveDataLength;	///  *** usbh_usr.c ****   if 0 stop
extern __IO uint8_t Count;		/// use!!!! ///stm32f4xx_it.c   for user button press! Count's value is 0 or1.
extern __IO uint8_t RepeatState ;	///main.c
extern __IO uint8_t LED_Toggle ;	///stm32f4xx_it.c Y3 G4 R5 B6
extern __IO uint8_t PauseResumeStatus ;
extern uint32_t AudioRemSize; 	/// use!!!! ///stm32f4_discovery_audio_codec.c
extern __IO uint32_t XferCplt;

///////// end of extern ///"waveplay.c"



/* Private variables ---------------------------------------------------------*/
AUDIO_PlaybackBuffer_Status BufferStatus; /* Status of buffer */
s16 AudioBuffer[4608];          /* Playback buffer - Value must be 4608 to hold 2xMp3decoded frames  +++  WAV buffer*/	 /* this is "16" bits!!!*/
static HMP3Decoder hMP3Decoder; /* Content is the pointers to all buffers and information for the MP3 Library */
MP3FrameInfo mp3FrameInfo;      /* Content is the output from MP3GetLastFrameInfo, 
                                   we only read this once, and conclude it will be the same in all frames
                                   Maybe this needs to be changed, for different requirements */
volatile uint32_t bytesLeft;    /* Saves how many bytes left in readbuf	*/
volatile uint32_t outOfData;    /* Used to set when out of data to quit playback */
volatile AUDIO_PlaybackBuffer_Status  audio_buffer_fill ; 
AUDIO_Playback_status_enum AUDIO_Playback_status ;
AUDIO_Format_enum AUDIO_Format ;
s8  *Audio_buffer;
uint8_t readBuf[READBUF_SIZE];       /* Read buffer where data from SD card is read to  +++ 5 * 1024 bytes*/
uint8_t *readPtr;                    /* Pointer to the next new data */
/* file system*/
FATFS fs;                   /* Work area (file system object) for logical drive */
FIL mp3FileObject;          /* file objects */
FRESULT res;                /* FatFs function common result code	*/
UINT n_Read;                /* File R/W count */
/* MP3已播放字节 */
uint32_t MP3_Data_Index;	
/* OS计数信号量 */	
///extern OS_EVENT *DMAComplete;
///extern OS_EVENT *StopMP3Decode;

///DMA_Configuration((s8 *)AudioBuffer=ADDR,(mp3FrameInfo.outputSamps*2)=size); 
///volatile uint8_t  ADDR; //= (s8 *)AudioBuffer=ADDR;
volatile uint16_t SIZE, SIZEinit; //= (mp3FrameInfo.outputSamps*2);
volatile uint16_t AP ; //
volatile uint8_t MF = 0; // MFが、0の邰udioBuffからデ〖タを粕み惧疤8ビットをg脱する

AUDIO_Playback_status_enum AUDIO_Playback_status ;
AUDIO_Format_enum AUDIO_Format ;
AUDIO_Length_enum AUDIO_Length ;
AUDIO_PlaybackBuffer_Status bufferstatus_local;

#define ABUFFSIZE  4608
#define	_BV(n)		( 1 << (n) )

/****/

static
int strstr_ext (
	const char *src,
	const char *dst
)
{
	int si, di;
	char s, d;

	si = strlen(src);
	di = strlen(dst);
	if (si < di) return 0;
	si -= di; di = 0;
	do {
		s = src[si++];
		if (s >= 'a' && s <= 'z') s -= 0x20;
		d = dst[di++];
		if (d >= 'a' && d <= 'z') d -= 0x20;
	} while (s && s == d);
	return (s == d);
}



/*------------------------------------------*/
/* Opens a file with associated method      */

void load_file (
	const char *fn,		/* Pointer to the file name */
	void *work,			/* Pointer to filer work area (must be word aligned) */
	UINT sz_work		/* Size of the filer work area */
)
{
	FIL fil;			/* Pointer to a file object */


	if (f_open(&fil, fn, FA_READ) == FR_OK) {
#if 0
		if (strstr_ext(fn, ".BMP")) {	/* BMP image viewer (24bpp, 1280pix in width max) */
			load_bmp(&fil, work, sz_work);
		}
		if (strstr_ext(fn, ".JPG")) {	/* Motion image viewer */
			load_jpg(&fil, work, sz_work);
		}
		if (strstr_ext(fn, ".IMG")) {	/* Motion image viewer */
			load_img(&fil, work, sz_work);
		}
		if (strstr_ext(fn, ".WAV")) {	/* Sound file player (RIFF-WAVE only) */
			load_wav(&fil, fn, work, sz_work);
		}
#endif
		if (strstr_ext(fn, ".MP3")) {	/* mp3 player */
			f_close(&fil);
			PlayAudioFile(&fil, fn);
		}

		f_close(&fil);

	}
}



///////////////////////////////////////
/*--------------------------------------------------------------------------*/
/* Monitor                                                                  */
/*--------------------------------------------------------------------------*/
DWORD acc_size;				/* Work register for fs command */
WORD acc_files, acc_dirs;
FILINFO Finfo;

static
FRESULT scan_files (
    char* path        /* 奐巒僲乕僪 (儚乕僋僄儕傾偲偟偰傕巊梡) */
)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
    char *fn;   /* 旕Unicode峔惉傪憐掕 */
	char str[255];

#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif


    res = f_opendir(&dir, path);                       /* 僨傿儗僋僩儕傪奐偔 */
    if (res == FR_OK) {
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* 僨傿儗僋僩儕崁栚傪1屄撉傒弌偡 */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* 僄儔乕傑偨偼崁栚柍偟偺偲偒偼敳偗傞 */
            if (fno.fname[0] == '.') continue;             /* 僪僢僩僄儞僩儕偼柍帇 */
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            if (fno.fattrib & AM_DIR) {                    /* 僨傿儗僋僩儕 */
                sprintf(&path[i], "/%s", fn);
                scan_files(path);
                ///if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* 僼傽僀儖 */
				sprintf(str,"%s/%s", path, fn);
                load_file( str, Buff, sizeof Buff);
				
            }
        }
    }

    return res;
}

/*******************************************************************************
* Function Name  : AudioStart
* Description    : Entry for mp3 player 
* Input          : 
* Output         : None
* Return         : 
* Attention		 : 
*******************************************************************************/
void AudioStart()
{
	char path[] = "0:";

#if 0
    FILINFO fno;
    DIR dir;
    int i;
    char *fn;   /* 旕Unicode峔惉傪憐掕 */
#endif
#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

#if 0
    res = f_opendir(&dir, path);                       /* 僨傿儗僋僩儕傪奐偔 */
    if (res == FR_OK) {
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* 僨傿儗僋僩儕崁栚傪1屄撉傒弌偡 */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* 僄儔乕傑偨偼崁栚柍偟偺偲偒偼敳偗傞 */
            if (fno.fname[0] == '.') continue;             /* 僪僢僩僄儞僩儕偼柍帇 */
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            if (fno.fattrib & AM_DIR) {                    /* 僨傿儗僋僩儕 */
                sprintf(&path[i], "/%s", fn);
                scan_files(path);
                ///if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* 僼傽僀儖 */
				////sprintf(str,"%s/%s", path, fn);
                ////load_file( str, Buff, sizeof Buff);
				
            }
        }
    }
#endif

	f_mount(0, &fs);

#if 0
	scan_files("0:/48k");
	scan_files("0:/mp3");	// ador dorath
 	scan_files("0:/music");
#endif

	scan_files(path);

}

/*******************************************************************************
* Function Name  : AUDIO_PlaybackBuffer_Status
* Description    : Gets the status of Playback buffer
* Input          : - value: 0 = get the current status, else reset the flag
*                           passed in parameter
* Output         : None
* Return         : FULL=0, LOW_EMPTY=1, HIGH_EMPTY=2
* Attention		 : None
*******************************************************************************/
AUDIO_PlaybackBuffer_Status AUDIO_PlaybackBuffer_GetStatus(AUDIO_PlaybackBuffer_Status value)
{
    if ( value )
        audio_buffer_fill &= ~value;
    return audio_buffer_fill; 
}

/*******************************************************************************
* Function Name  : AUDIO_Playback_Stop
* Description    : Stop the playback by stopping the DMA transfer
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void AUDIO_Playback_Stop(void)
{
///   DMA_Cmd(DMA1_Channel5, DISABLE);                    /* Disable DMA Channel for Audio */
/// DAC_Cmd(  SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);   /* Disable I2S DMA REQ */    
///  	DAC_Cmd(DAC_Channel_1, DISABLE);
///  	DAC_Cmd(DAC_Channel_2, DISABLE);
   	TIM_Cmd(TIM6, DISABLE);

   	AUDIO_Playback_status = NO_SOUND;       

   /* Shutdwon codec in order to avoid non expected voices */
///   codec_send( ACTIVE_CONTROL | INACTIVE );    /* WM8731 Disable */
   MP3_Data_Index = 0;
///   OSSemPost(DMAComplete);	     /* 发送信号量 */
}

/*******************************************************************************
* Function Name  : FillBuffer
* Description    : Fill the buffer with Music from MP3 Data
* Input          : - FileObject: file system 
*                  - Start: If 1, we will fill buffer nomatter the status of the playback,
*                           used in beginning of file (pre-fill)
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
int FillBuffer(FIL *FileObject , uint8_t start)  
{
    uint8_t word_Allignment; /* Variable used to make sure DMA transfer is alligned to 32bit */                
    int err=0;           /* Return value from MP3decoder */
    int offset;          /* Used to save the offset to the next frame */
    
    if(!start) {   /* If we are in start pos we overwrite bufferstatus */ 
        BufferStatus = AUDIO_PlaybackBuffer_GetStatus( (AUDIO_PlaybackBuffer_Status) 0 );
		///GPIOD->BRR = _BV(4); ///LED on
 
    } else {
        BufferStatus = (AUDIO_PlaybackBuffer_Status) ( LOW_EMPTY | HIGH_EMPTY );    
		///GPIOD->BRR = _BV(4); ///LED off
	}
        /* somewhat arbitrary trigger to refill buffer - should always be enough for a full frame */  
		if (bytesLeft < 2*MAINBUF_SIZE /*&& !eofReached*/) { 
          	/* move last, small chunk from end of buffer to start, then fill with new data */ 
          	word_Allignment = 4 - (bytesLeft % 4 );   /* Make sure the first byte in writing pos. is word alligned */ 
          	memmove(readBuf+word_Allignment, readPtr, bytesLeft);
          	res = f_read( FileObject, (uint8_t *)readBuf+word_Allignment + bytesLeft,READBUF_SIZE - bytesLeft - word_Allignment,&n_Read);
		 
		  	if( n_Read !=0 ) {
		     	MP3_Data_Index += n_Read;   /* 记录已播放大小 */
             	/* zero-pad to avoid finding false sync word after last frame (from old data in readBuf) */
             	if (n_Read < READBUF_SIZE - bytesLeft - word_Allignment) {    
		        	memset(readBuf + bytesLeft + n_Read, 0, READBUF_SIZE - bytesLeft - n_Read);
		     	}	          
             	bytesLeft += n_Read;
		     	readPtr = readBuf+word_Allignment;
		  	} else {
		     	outOfData = 1;
		  	}		  	 		  		 
		}  

        if(outOfData == 1) {
        	/* Stop Playback */  
            AUDIO_Playback_Stop();
            return 0; 
        }

		/* find start of next MP3 frame - assume EOF if no sync found */
		offset = MP3FindSyncWord(readPtr, bytesLeft);   
		if (offset < 0) {
		    bytesLeft = 0;
		    return -1;
		}
    
		readPtr += offset;			  /* data start point */
		bytesLeft -= offset;		  /* in buffer */
		if (BufferStatus & LOW_EMPTY)  {/* Decode data to low part of buffer */
 		   err = MP3Decode(hMP3Decoder, &readPtr, (int *)&bytesLeft, (short *)AudioBuffer, 0); 	
 		   BufferStatus = AUDIO_PlaybackBuffer_GetStatus(LOW_EMPTY);
 			////printf("L");	///
        }
 		else if (BufferStatus & HIGH_EMPTY) {    /* Decode data to the high part of buffer */
		   err = MP3Decode(hMP3Decoder, &readPtr, (int *)&bytesLeft, (short *)AudioBuffer+mp3FrameInfo.outputSamps,0);	
		   BufferStatus = AUDIO_PlaybackBuffer_GetStatus(HIGH_EMPTY);	 			 
			////printf("H");	////
        } 
        if (err!=0) {  
            bytesLeft=0;
            readPtr = readBuf;
			////printf("-- MP3Decode Error Occurred %s \r\n",MP3_GetStrResult(err));
            /* error occurred */
            switch (err) /* There is not implemeted any error handling. it will quit the playing in case of error */
            {
                /* do nothing - next call to decode will provide more mainData */
                default:
				  return -1; 				  
            }
        }     
  return 0;
}

/*******************************************************************************
* Function Name  : PlayAudioFile
* Description    : None
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void PlayAudioFile(FIL *FileObject,const char *path)
{
   ///INT8U err;
   uint8_t err;
   /* Reset counters */
   bytesLeft = 0;   
   outOfData = 0;   

   readPtr = readBuf;  
   hMP3Decoder = MP3InitDecoder();
   
   res = f_open(FileObject , path , FA_OPEN_EXISTING | FA_READ);		
   ////printf("res = f_open(FileObject=0x%X , path=%s ...res=%d\r\n)",&FileObject, &path,res);
   memset(&mp3_info,0,sizeof(mp3_info));
      			
   Read_ID3V2(FileObject,&mp3_info);
   
   Read_ID3V1(FileObject,&mp3_info);
    
   MP3_Data_Index = 0;

   mp3_info.data_start = 0;

   /* 播放时间 */
   if( (strstr(path,"MP3") !=NULL) || (strstr(path,"mp3") !=NULL) )
   {																					   
	   if( (strstr(path,"MP3") !=NULL) )
	   {
	      mp3_info.duration = ( ( path[ strstr(strstr(path,"MP3") + 5,":") - path - 2 ] - '0' ) *10  + \
	                            ( path[ strstr(strstr(path,"MP3") + 5,":") - path - 1 ] - '0' )  ) * 60; /* 分钟 */

	      mp3_info.duration = mp3_info.duration + ( path[ strstr(strstr(path,"MP3") + 5,":") - path + 1 ] - '0' ) *10 + \
	                                              ( path[ strstr(strstr(path,"MP3") + 5,":") - path + 2 ] - '0' );	 /* 秒   */
	   }
	   else if( (strstr(path,"mp3") !=NULL) )
	   {
	      mp3_info.duration = ( ( path[ strstr(strstr(path,"mp3") + 5,":") - path - 2 ] - '0' ) *10  + \
	                            ( path[ strstr(strstr(path,"mp3") + 5,":") - path - 1 ] - '0' )  ) * 60; /* 分钟 */

	      mp3_info.duration = mp3_info.duration + ( path[ strstr(strstr(path,"mp3") + 5,":") - path + 1 ] - '0' ) *10 + \
	                                              ( path[ strstr(strstr(path,"mp3") + 5,":") - path + 2 ] - '0' );	 /* 秒   */
	   }	   
   }

   mp3_info.position = -1;

   res = f_lseek(FileObject,mp3_info.data_start); 

   /* Read the first data to get info about the MP3 File */
   while( FillBuffer( FileObject  , 1 ) ); 	 /* Must Read MP3 file header information */
   /* Get the length of the decoded data, so we can set correct play length */                        
   MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo); 
  
   /* Select the correct samplerate and Mono/Stereo */
   AUDIO_Init_audio_mode( ( AUDIO_Length_enum )mp3FrameInfo.bitsPerSample,  \
						((MP3DecInfo *)hMP3Decoder)->samprate,  \
						(((MP3DecInfo *)hMP3Decoder)->nChans==1) ? MONO : STEREO);

   /* Start the playback */
   AUDIO_Playback_status = IS_PLAYING;   
   WaveDataLength = (Finfo.fsize-(4+128)) * ( mp3_info.bit_rate / 8) * mp3_info.sampling * (AUDIO_Format+1) ;
///   while( !(outOfData == 1) && !OSSemAccept(StopMP3Decode) )
   ///while( !(outOfData == 1) )
   while(  !(outOfData == 1) &&  HCD_IsDeviceConnected(&USB_OTG_Core))
   {   
      ////OSSemPend(DMAComplete,0,&err);	  /* 获取信号量 */
      FillBuffer(FileObject ,0 );
  	  if (WaveDataLength == 0) break ;
		
   } 
   	/* release mp3 decoder */
    MP3FreeDecoder(hMP3Decoder);
    WavePlayerStop();

}

/*******************************************************************************
* Function Name  : MP3_GetStrResult
* Description    : Convert file access error number in string
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
const char* MP3_GetStrResult(int err)
{
    switch(err) {
        case ERR_MP3_NONE :                   return "ERR_MP3_NONE";
        case ERR_MP3_INDATA_UNDERFLOW :       return "ERR_MP3_INDATA_UNDERFLOW";
        case ERR_MP3_MAINDATA_UNDERFLOW :     return "ERR_MP3_MAINDATA_UNDERFLOW";
        case ERR_MP3_FREE_BITRATE_SYNC :      return "ERR_MP3_FREE_BITRATE_SYNC";
        case ERR_MP3_OUT_OF_MEMORY :          return "ERR_MP3_OUT_OF_MEMORY";
        case ERR_MP3_NULL_POINTER :           return "ERR_MP3_NULL_POINTER";
        case ERR_MP3_INVALID_FRAMEHEADER :    return "ERR_MP3_INVALID_FRAMEHEADER";   
        case ERR_MP3_INVALID_SIDEINFO :       return "ERR_MP3_INVALID_SIDEINFO"; 
        case ERR_MP3_INVALID_SCALEFACT :      return "ERR_MP3_INVALID_SCALEFACT";
        case ERR_MP3_INVALID_HUFFCODES :      return "ERR_MP3_INVALID_HUFFCODES";
        case ERR_MP3_INVALID_DEQUANTIZE :     return "ERR_MP3_INVALID_DEQUANTIZE";
        case ERR_MP3_INVALID_IMDCT :          return "ERR_MP3_INVALID_IMDCT";
        case ERR_MP3_INVALID_SUBBAND :        return "ERR_MP3_INVALID_SUBBAND";
        case ERR_UNKNOWN :                    return "ERR_UNKNOWN";  
        default :                             return "?";
    }  
}

#if 0
/*******************************************************************************
* Function Name  : DMA_Configuration
* Description    : DMA1 channel5 configuration
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void DMA_Configuration( s8 * addr, int size)
{
    DMA_InitTypeDef DMA_InitStructure; 
   /* DMA2 Channel2 configuration ----------------------------------------------*/
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	/* DMA Channel configuration ----------------------------------------------*/
	DMA_Cmd(DMA1_Channel5, DISABLE);

    /* Uses the original buffer	*/            
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) addr;    /* Set the buffer */
	DMA_InitStructure.DMA_BufferSize = size;			       /* Set the size */
  
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;	 /* DMA循环模式 */
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);  
	/* Enable SPI DMA Tx request */
    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC | DMA_IT_HT, ENABLE);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA1_Channel5, ENABLE);
}
#endif

#if 0
/*******************************************************************************
* Function Name  : I2S_Configuration
* Description    : I2S2 configuration
* Input          : - I2S_AudioFreq: 采样频率
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void I2S_Configuration(uint32_t I2S_AudioFreq)
{
   I2S_InitTypeDef I2S_InitStructure;
   GPIO_InitTypeDef GPIO_InitStructure;
   
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE); 
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE); 

   /* WS */
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
   GPIO_Init(GPIOB, &GPIO_InitStructure);

   /* CK	*/
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
   GPIO_Init(GPIOB, &GPIO_InitStructure);

   /* SD	*/
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
   GPIO_Init(GPIOB, &GPIO_InitStructure);
	
   SPI_I2S_DeInit(SPI2);
							
   I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;
   I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
   I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
   I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq;
   I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
  
   /* I2S3 Master Transmitter to I2S2 Slave Receiver communication ------------*/
   /* I2S3 configuration */
   I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;

   I2S_Init(SPI2, &I2S_InitStructure);

   /* Enable the I2S2 */
   I2S_Cmd(SPI2, ENABLE);
}
#endif

#if 0
/*******************************************************************************
* Function Name  : DMA2_Channel2_IRQHandler
* Description    : Handles the DMA2 global interrupt request 
* Input          : None
* Output         : None
* Return         : None
* Attention		 : in mono mode, stop managed by AUDIO_Cpy_Mono() else it must 
*                  be managed by the application
*******************************************************************************/
void DMA1_Channel5_IRQHandler(void)   
{
   CPU_SR         cpu_sr;
   OS_ENTER_CRITICAL();
   OSIntNesting++;
   OS_EXIT_CRITICAL();	
   	
   if(DMA_GetITStatus(DMA1_IT_HT5))	 /* DMA1 通道5 半传输中断 */
   {
     DMA_ClearITPendingBit(DMA1_IT_GL5|DMA1_IT_HT5);  /* DMA1 通道5 全局中断 */
     audio_buffer_fill |= LOW_EMPTY;
	 OSSemPost(DMAComplete);	     /* 发送信号量 */
   }  
        
   if(DMA_GetITStatus(DMA1_IT_TC5))  /* DMA1 通道5 传输完成中断 */
   {
     DMA_ClearITPendingBit(DMA1_IT_GL5|DMA1_IT_TC5); /* DMA1 通道5 全局中断 */
     audio_buffer_fill |= HIGH_EMPTY;
	 OSSemPost(DMAComplete);	     /* 发送信号量 */
   } 

   OSIntExit(); 
}
#endif

#if 0
/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures DMA IRQ channel.
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
static void NVIC_Configuration(void)
{
   NVIC_InitTypeDef NVIC_InitStructure;

   /* DMA IRQ Channel configuration */
   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
   NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
}
#endif

void set_isp_pll(uint32_t frequency)
{
	int I2S_N, I2S_R; 
	if (frequency == 48000) {
       I2S_N = 258;
       I2S_R =   3;
	
	} else {
       I2S_N = 271;
       I2S_R =   2;
	}

/******************************************************************************/
/*                          I2S clock configuration                           */
/******************************************************************************/
  /* PLLI2S clock used as I2S clock source */
  RCC->CFGR &= ~RCC_CFGR_I2SSRC;

  /* Configure PLLI2S */
  RCC->PLLI2SCFGR = (I2S_N << 6) | (I2S_R << 28);

  /* Enable PLLI2S */
  RCC->CR |= ((uint32_t)RCC_CR_PLLI2SON);

  /* Wait till PLLI2S is ready */
  while((RCC->CR & RCC_CR_PLLI2SRDY) == 0)
  {
  }

}


/*******************************************************************************
* Function Name  : AUDIO_Init_audio_mode
* Description    : Initialization for Audio Mode use of the WM8731 codec.set WM8731
*                  to Audio Mode via I2C. enable STM32 I2S communication to send
*                  audio samples (SPI2/I2S2 port) in DMA mode
* Input          : - length: 8 bits or 16 bits length sample
*				   - frequency:	8 KHz, 16 KHz, 22 KHz, 44 KHz, 48 KHz sample
*				   - format: mono, stereo
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void AUDIO_Init_audio_mode(AUDIO_Length_enum length, uint16_t frequency, AUDIO_Format_enum format)
{
   /* The MONO mode uses working buffer to dupplicate datas on the two channels
      and switch buffer half by half => uses DMA in circular mode */
   
   mp3_info.length = length;
   mp3_info.sampling = frequency;
   mp3_info.bit_rate = mp3FrameInfo.bitrate;

   AUDIO_Playback_status = NO_SOUND;
   AUDIO_Format = format;

   /* Buffers are supposed to be empty here	*/
   audio_buffer_fill = (AUDIO_PlaybackBuffer_Status) ( LOW_EMPTY | HIGH_EMPTY ) ;

	set_isp_pll((uint32_t) frequency);
 	WavePlayerInit((uint32_t) frequency);
	AudioRemSize   = 0; 
  	XferCplt = 0;		/// if you use USER button,so this Flag is needed
  	LED_Toggle = 6;
  	PauseResumeStatus = 1;
  	Count = 0;
	///Audio_MAL_Play((uint32_t)buffer2, _MAX_SS);
	Audio_MAL_Play((uint32_t)AudioBuffer, mp3FrameInfo.outputSamps*4);
	///EVAL_AUDIO_Play(AudioBuffer, (uint32_t) mp3FrameInfo.outputSamps*16);
    
///   NVIC_Configuration();
///   I2S_Configuration(frequency);  
///   codec_send( ACTIVE_CONTROL | ACTIVE );	   /* WM8731 Enable */
///   DMA_Configuration((s8 *)AudioBuffer,(mp3FrameInfo.outputSamps*2)); 
///	printf("mp3FrameInfo.outputSamps*2=%d \r\n",mp3FrameInfo.outputSamps*2); ///mp3FrameInfo.outputSamps*2=4608
#if 0
	MP3_NVIC_Config4TIM6();
	DAC_Config2();
	////dac_outMP3();

   if ( strlen(mp3_info.title) ==0 )
   {
      strcpy( mp3_info.title, "mp3_info.title");  
   }

   if ( strlen(mp3_info.title) > 20 )
   {
      strcpy( mp3_info.title, "mp3_info.title");  
   }

   printf( "\r\n-- MP3 file header information \r\n");
   printf( "---------------------------------------------------\r\n");
   printf( "-- MP3 title          : %s \r\n", mp3_info.title);	  
   printf( "-- MP3 artist         : %s \r\n", mp3_info.artist);	  
   printf( "-- MP3 bitrate        = %d \r\n", mp3_info.bit_rate);	       /* 比特率 */
   printf( "-- MP3 bitsPerSample  = %d \r\n", mp3_info.length);			   /* 每个采样需要的bit数 */
   printf( "-- MP3 samprate       = %d \r\n", mp3_info.sampling);  		   /* 采样率 */
   printf( "-- MP3 nChans         = %d \r\n", AUDIO_Format);  			   /* 声道 */
   /* 每帧数据的采样数 */
   printf( "-- MP3 nGranSamps     = %d \r\n", ((MP3DecInfo *)hMP3Decoder)->nGranSamps);	 
   /* 数据帧大小 FrameSize = (((MpegVersion == MPEG1 ? 144 : 72) * Bitrate) / SamplingRate) + PaddingBit */
   printf( "-- MP3 outputSamps    = %d \r\n", mp3FrameInfo.outputSamps);  
   printf( "-- MP3 layer          = %d \r\n", mp3FrameInfo.layer);
   printf( "-- MP3 version        = %d \r\n", mp3FrameInfo.version);
   printf( "-- MP3 duration       = %X \r\n", mp3_info.duration);
/*
-- MP3 file header information
---------------------------------------------------
-- MP3 title          : Dor Daedeloth
-- MP3 artist         : Summoning
-- MP3 bitrate        = 320000
-- MP3 bitsPerSample  = 16
-- MP3 samprate       = 44100
-- MP3 nChans         = 1
-- MP3 nGranSamps     = 576
-- MP3 outputSamps    = 2304
-- MP3 layer          = 3
-- MP3 version        = 0
-- MP3 duration       = FFFF897E

*/   
	MP3_TIM6_Configuration(mp3_info.sampling);
#endif  
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/

#if 0
	/*********************************************************************************************************
      Add 柒垄DAC 12bit version by shuji009
*********************************************************************************************************/
void MP3_NVIC_Config4TIM6(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;


    /* Enable the TIM6 gloabal Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel                   = TIM6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;

    NVIC_Init(&NVIC_InitStructure);

}
#endif
#if 0
/*--------------------------------------------------------------*/
/* timer6()			    インタ〖バル(CTC)                       */
/*--------------------------------------------------------------*/
///http://www.kimura-lab.net/wiki/index.php/STM32%E3%81%A7%E3%82%BF%E3%82%A4%E3%83%9E%E3%82%92%E4%BD%BF%E3%81%A3%E3%81%A6%E3%81%BF%E3%82%8B
//48000Hz	20.83333333us	1500
//44100Hz	22.67573696us	1632.653061
//22050Hz	45.35147392us	3265.306122
///#define SYSFREQ		72000000UL
#define SYSFREQ		108000000UL
#define TP			10
void MP3_TIM6_Configuration(uint32_t freq)	/* freq 48000(Hz)/44100(Hz)/22050(Hz).... etc */
{
	//////////////////////////////////////////////////////////////////////////
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

	///printf("freq=%d \r\n",freq);	///debug
    //////TIM_Cmd(TIM6, DISABLE);
	///// 72MHz -> 18MHz -> 1kHz
  	/* Time base configuration */
  	TIM_TimeBaseStructure.TIM_Period            = (uint16_t)((SYSFREQ / 2) * 2 / freq);
  	TIM_TimeBaseStructure.TIM_Prescaler         = 0;
  	TIM_TimeBaseStructure.TIM_ClockDivision     = 0;
  	TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up;
  	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    // 肋年を瓤鼻
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
	///printf("TIM_TimeBaseInit done (uint16_t)((SYSFREQ / 2) * 2 / freq)=%d \r\n",(uint16_t)((SYSFREQ / 2) * 2 / freq));	///debug

    // 充り哈み钓材...充り哈み妥傍のORを苞眶に掐れる
    TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
	///printf("0. TIM6->DIER=0 \r\n");	///debug
	////GPIOD->BRR = _BV(4); //LED on
	////TIM6->DIER |= TIM_IT_Update;	//TIMx->DIER |= TIM_IT;	 == TIM_Cmd(TIM6, ENABLE);
	///printf("1. TIM6->DIER=0x%x done \r\n",TIM_IT_Update);	///debug

//#if 0
    // タイマ6弹瓢侍数恕
    TIM6->CR1 |= TIM_CR1_CEN;	
	TIM_Cmd(TIM6, ENABLE);
	///printf("2. TIM6->CR1=0x%X,$%X \r\n",TIM6->CR1,&(TIM6->CR1));	///debug
//#endif
	///printf("timer6 for MP3 start ---\r\n");	///debug

	TIM_Cmd(TIM6, ENABLE);

}
#endif
#if 0
	////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
uint8_t  led_on=0;
uint16_t countti=0,ccc=0;

void TIM6_IRQHandler(void)	// output DAC 2ch
{
	////printf("!");	///debug
    if( TIM_GetITStatus( TIM6, TIM_IT_Update) != RESET) {
    	////printf("*");	///debug
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
    }
//#if 0
	if(led_on) {
		GPIOD->BRR = _BV(4); //LED on
		led_on = 1;
	} else {
		GPIOD->BSRR = _BV(4); //LED off
		led_on = 0;
	}
///#else
//#endif

   	dac_outMP3();	//叫蜗
	///GPIOD->BSRR = _BV(4); //LED off
	///GPIOD->BRR = _BV(4); //LED on
	///countti++;
	///if( (countti % 44100) == 0) printf("*"); 	//OK
///#endif

}
#endif
#if 0
#define wavbuff AudioBuffer
#define ap AP
///#define ap++ AP++
void dac_outMP3(void)
{
	uint16_t x, y, d;

		///x = AudioBuffer[AP++];
		///y = AudioBuffer[AP++];
	
	if (mp3_info.length == LG_16_BITS && AUDIO_Format == STEREO ) {
		x = 0x8000 - AudioBuffer[AP++];
		///d = (x>>8 & 0xFF) | (x<<8 & 0xFF00);x=d;
		y =  0x8000 - AudioBuffer[AP++];
		///d = (y>>8 & 0xFF) | (y<<8 & 0xFF00);y=d;

		///if(countti == 0)	printf(" 16ST ");//////


	} else if (mp3_info.length == LG_16_BITS && AUDIO_Format == MONO ) {
		x = AudioBuffer[AP++];
		y = x;
		////if(countti == 0)	printf(" 16MN");//////
	} else if (mp3_info.length == LG_8_BITS && AUDIO_Format == STEREO ) {
		d = AudioBuffer[AP++];
		x = (uint16_t) (d & 0xFF00);
		y = (uint16_t) ((d & 0xFF) << 8);
		////if(countti == 0)	printf(" 8ST ");//////
	} else if (mp3_info.length == LG_8_BITS && AUDIO_Format == MONO ) {
		if(MF == 0) {
			d = AudioBuffer[AP];	// ポインタはそのまま
			x = (uint16_t) (d & 0xFF00); y = x;
			MF = 1;
		} else if (MF == 1) {
			d = AudioBuffer[AP++];	// ポインタを渴める
			x = (uint16_t) ((d & 0xFF) << 8); y = x;
			MF = 0;
		}
		////if( countti ==0)	printf(" 8MN ");//////

	}
	///printf("x,y:%x,%x ",y,x);
	DAC_SetDualChannelData(DAC_Align_12b_L, (uint32_t)y, (uint32_t)x);
    ///DAC_SetDualChannelData( DAC_Align_12b_L, (uint32_t)y * 887 / 1000 + 250, (uint32_t)x * 887 / 1000 + 250 );

//#if xxx
	countti++;
	/////if( (countti % 512) == 0) printf("AP=%d  ",AP); 	//OK
	if (countti % 44100 == 0) { 
		//printf("\r\nAUDIO_Length=%d , AUDIO_Format=%d \r\n",mp3_info.length, AUDIO_Format); 	//OK
		////printf("x=0x%04X , y=0x%04X  ",x,y); 	//OK

		if(led_on) {
			GPIOD->BRR = _BV(4); //LED on
			led_on = 0;
		} else {
			GPIOD->BSRR = _BV(4); //LED off
			led_on = 1;
		}

	}
//#endif
	////if (AP == (ABUFFSIZE>>1) && MF == 0 ) {	// (AP == ((ABUFFSIZE>>1) + 1) && MF == 0) {
	if (AP == (ABUFFSIZE>>1) ) {	// (AP == ((ABUFFSIZE>>1) + 1) && MF == 0) {
		bufferstatus_local |= LOW_EMPTY;
		audio_buffer_fill |= LOW_EMPTY;
		////printf(" Low Empty ");	////   ok
	} else if (AP == (ABUFFSIZE)   ) {
		bufferstatus_local |= HIGH_EMPTY;
		audio_buffer_fill |= HIGH_EMPTY;
		AP = 0;
		////printf(" High Empty ");	////   ok
	}
}
#endif
#if 0
// ステレオでもモノラルでも2chをg脱する
volatile void DAC_Config2(void)
{
  GPIO_InitTypeDef		GPIO_InitStructure;
  DAC_InitTypeDef		DAC_InitStructure;

 	// GPIOdac惦排倡n PA4/PA5
	////RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//稍妥かも	 msinでやっている
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	// DAC惦排倡n
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_5; 	// PA4:DAC_OUT1 PA5:DAC_OUT2
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//纳裁
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* DAC channel1 Configuration */
  DAC_InitStructure.DAC_Trigger        = DAC_Trigger_None;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_OutputBuffer   = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_1, &DAC_InitStructure);
  DAC_Init(DAC_Channel_2, &DAC_InitStructure);

  /* Enable DAC Channel1: Once the DAC channel1 is enabled, PA.04 is
     automatically connected to the DAC converter. */
  DAC_Cmd(DAC_Channel_1, ENABLE);
  DAC_Cmd(DAC_Channel_2, ENABLE);

  ////printf("\r\nDAC config end\r\n");
								 
}
#endif
