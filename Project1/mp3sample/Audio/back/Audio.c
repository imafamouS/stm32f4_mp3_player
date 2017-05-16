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
#include <audio.h>

/// add	audio buff size
#define ABUFFSIZE  4608


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

BYTE Buff[4096] __attribute__ ((aligned(4)));

/* Private variables ---------------------------------------------------------*/
AUDIO_PlaybackBuffer_Status BufferStatus; /* Status of buffer */
s16 AudioBuffer[ABUFFSIZE];     /* Playback buffer - Value must be 4608 to hold 2xMp3decoded frames +++ WAVE buffer */
/////////s16 AudioBuffer[];     /* Playback buffer - Value must be 4608 to hold 2xMp3decoded frames +++ WAVE buffer */
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
uint8_t readBuf[READBUF_SIZE];       /* Read buffer where data from SD card is read to +++ 5 * 1024/for foo.MP3 */ 
uint8_t *readPtr;                    /* Pointer to the next new data +++ for foo.MP3 */
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
volatile uint8_t  ADDR; //= (s8 *)AudioBuffer=ADDR;
volatile uint16_t AP ; //
volatile uint8_t MF = 0; // MF偑丄0偺濧udioBuff偐傜僨乕僞傪撉傒忋埵8價僢僩傪g梡偡傞

AUDIO_Playback_status_enum AUDIO_Playback_status ;
AUDIO_Format_enum AUDIO_Format ;
AUDIO_Length_enum AUDIO_Length ;
AUDIO_PlaybackBuffer_Status bufferstatus_local;

#define	_BV(n)		( 1 << (n) )



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
		if (strstr_ext(fn, ".JPG")) {	/* Motion image viewer */
			load_jpg(&fil, work, sz_work);
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
	char* path		/* Pointer to the path name working buffer */
)
{
	DIR dirs;
	FRESULT res;
	BYTE i;
	char* fn;
	char str[20];


	if ((res = f_opendir(&dirs, path)) == FR_OK) {
		i = strlen(path);
		while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) {
			if (_FS_RPATH && Finfo.fname[0] == '.') continue;
#if _USE_LFN
			fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
			fn = Finfo.fname;
#endif
			sprintf(str,"%s/%s",path, fn);
			///printf("%s/%s\r\n",path,fn);
			load_file( str, Buff, sizeof Buff);
			if (Finfo.fattrib & AM_DIR) {
				acc_dirs++;
				*(path+i) = '/'; strcpy(path+i+1, fn);
				res = scan_files(path);
				*(path+i) = '\0';
				if (res != FR_OK) break;
			} else {
				printf("%s/%s\r\n", path, fn);
				acc_files++;
				acc_size += Finfo.fsize;
			}
		}
	}

	return res;
}




#if 0
-*-----------
	f_mount(0, &Fatfs);

	ptr="/music/folder.jpg";
	load_file(ptr,Buff, sizeof Buff);

	res = scan_files("mp3");
	res = scan_files("music");
-----------------------
    while(1)
    {
      STM_EVAL_LEDToggle(LED6);		//Blue -- miniUSB
	  	
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
      Delay(100000);
    }    

#endif
/*******************************************************************************
* Function Name  : AudioStart
* Description    : 
* Input          : 
* Output         : None
* Return         : 
* Attention		 : 
*******************************************************************************/
void AudioStart()  ////++ rntry
{
	FATFS Fatfs;

    ///LED_Toggle = 6;		//Blue LED

	f_mount(0, &Fatfs);


	scan_files("0:/mp3");	// ador dorath
	scan_files("0:/music");


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
///   	TIM_Cmd(TIM6, DISABLE);

   	AUDIO_Playback_status = NO_SOUND;       
	AudioPlayStart = 0;

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
 
    } else {
        BufferStatus = (AUDIO_PlaybackBuffer_Status) ( LOW_EMPTY | HIGH_EMPTY );    
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
		   LED_Toggle = 5;  ///	Y3 G4 R5 B6 red
		   LED_Toggle = 5;  ///	Y3 G4 R5 B6 red
		}
 		else if (BufferStatus & HIGH_EMPTY) {    /* Decode data to the high part of buffer */
           err = MP3Decode(hMP3Decoder, &readPtr, (int *)&bytesLeft, (short *)AudioBuffer+mp3FrameInfo.outputSamps,0);	
		   BufferStatus = AUDIO_PlaybackBuffer_GetStatus(HIGH_EMPTY);
           LED_Toggle = 4;  ///	Y3 G4 R5 B6 blue

        } 
        if (err!=0) {  
            bytesLeft=0;
            readPtr = readBuf;
			///printf("-- MP3Decode Error Occurred %s \r\n",MP3_GetStrResult(err));
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
    uint8_t word_Allignment; /* Variable used to make sure DMA transfer is alligned to 32bit */                

//	uint8_t err;
   /* Reset counters */
   bytesLeft = 0;   
   outOfData = 0;   


   readPtr = readBuf;  
   hMP3Decoder = MP3InitDecoder();	///// <-- NG point call helix: mp3dec.c

   res = f_open(FileObject , path , FA_OPEN_EXISTING | FA_READ);		
   ///printf("res = f_open(FileObject=0x%X , path=%s ...res=%d\r\n)",&FileObject, &path,res);
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
   	AudioPlayStart = 1;
  	WaveDataLength = 1024*1024*1024;
///    FillBuffer(FileObject ,1 ); ////		
///----------------
      STM_EVAL_LEDToggle(LED3);		//Yello
      STM_EVAL_LEDToggle(LED4);		//Green
///      STM_EVAL_LEDToggle(LED5);		//Red
///      STM_EVAL_LEDToggle(LED6);		//Blue -- miniUSB
///----------------------------
///	Audio_MAL_Play((uint32_t)AudioBuffer, ABUFFSIZE);	///+++

   while( !(outOfData == 1) ) {   
      FillBuffer(FileObject ,0 );
#ifdef ddd	  
	  if (audio_buffer_fill == (AUDIO_PlaybackBuffer_Status) ( LOW_EMPTY | HIGH_EMPTY ))  {
	      STM_EVAL_LEDToggle(LED5); 
	  }
#endif
	  		
   } 
      STM_EVAL_LEDOff(LED3);
      STM_EVAL_LEDOff(LED4);
      STM_EVAL_LEDOff(LED5);
      STM_EVAL_LEDOff(LED6);
   	/* release mp3 decoder */
    MP3FreeDecoder(hMP3Decoder);
	WaveDataLength = 0;	 
	f_close(FileObject);
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
    
///   NVIC_Configuration();
///   I2S_Configuration(frequency);  
///   codec_send( ACTIVE_CONTROL | ACTIVE );	   /* WM8731 Enable */
///   DMA_Configuration((s8 *)AudioBuffer,(mp3FrameInfo.outputSamps*2)); 
///	printf("mp3FrameInfo.outputSamps*2=%d \r\n",mp3FrameInfo.outputSamps*2); ///mp3FrameInfo.outputSamps*2=4608
 	WavePlayerInit((uint32_t) frequency);
	AudioRemSize   = 0; 
  	XferCplt = 0;		/// if you use USER button,so this Flag is needed
  	LED_Toggle = 6;
  	PauseResumeStatus = 1;
  	Count = 0;
	///Audio_MAL_Play((uint32_t)buffer2, _MAX_SS);
	Audio_MAL_Play((uint32_t)AudioBuffer, mp3FrameInfo.outputSamps*2);

#if 0
	MP3_NVIC_Config4TIM6();
	DAC_Config2();

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
   printf( "-- MP3 bitrate        = %d \r\n", mp3_info.bit_rate);	       /* 塯栔懭 */
   printf( "-- MP3 bitsPerSample  = %d \r\n", mp3_info.length);			   /* 抆岕壡櫃樻櫒媍bit曽 */
   printf( "-- MP3 samprate       = %d \r\n", mp3_info.sampling);  		   /* 壡櫃懭 */
   printf( "-- MP3 nChans         = %d \r\n", AUDIO_Format);  			   /* 暀媉 */
   /* 抆洘曽徾媍壡櫃曽 */
   printf( "-- MP3 nGranSamps     = %d \r\n", ((MP3DecInfo *)hMP3Decoder)->nGranSamps);	 
   /* 曽徾洘婑槦 FrameSize = (((MpegVersion == MPEG1 ? 144 : 72) * Bitrate) / SamplingRate) + PaddingBit */
   printf( "-- MP3 outputSamps    = %d \r\n", mp3FrameInfo.outputSamps);  
   printf( "-- MP3 layer          = %d \r\n", mp3FrameInfo.layer);
   printf( "-- MP3 version        = %d \r\n", mp3FrameInfo.version);
   printf( "-- MP3 duration       = %X \r\n", mp3_info.duration);
   ///MP3_TIM6_Configuration(mp3_info.sampling);
#endif   
  
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
