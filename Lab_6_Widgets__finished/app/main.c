#include <RTL.h>
#include <stdio.h>
#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "main.h"
#include "hal_led.h"
#include "hal_system.h"
#include "uartstdio.h"
#include "ustdlib.h"
#include "cmdline.h"
#include "sound.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include <string.h>
#include "grlib/canvas.h"
#include "grlib/slider.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "kitronix320x240x16_ssd2119_8bit.h"
#include "touch.h"
#include "set_pinout.h"

#define RIFF_CHUNK_ID_RIFF      0x46464952
#define RIFF_CHUNK_ID_FMT       0x20746d66
#define RIFF_CHUNK_ID_DATA      0x61746164
#define RIFF_TAG_WAVE           0x45564157
#define RIFF_FORMAT_UNKNOWN     0x0000
#define RIFF_FORMAT_PCM         0x0001
#define RIFF_FORMAT_MSADPCM     0x0002
#define RIFF_FORMAT_IMAADPCM    0x0011



#define INITIAL_VOLUME_PERCENT 60																											
#define NUM_LIST_STRINGS 48
const char *wave_pointer[NUM_LIST_STRINGS];   
#define MAX_FILENAME_STRING_LEN (8 + 1 + 3 + 1)
char g_pcFilenames[NUM_LIST_STRINGS][MAX_FILENAME_STRING_LEN];



OS_TID taskID_1, taskID_2, taskID_3;		// Task1 is a widget-task
											// Task2 is a audioplay-task (higher priority)
											// Task3 is a UART-task (uncommented)


// STATE MACHINE AUDIO PLAYER
/****************************/
typedef enum								
{	
	stop,
	play
	
} STATE_AUDIO;

STATE_AUDIO player_state = stop;		    // Initial-state state machine

static unsigned short start_flag;			// button start clicked
static unsigned short stop_flag;		    // button stop	clicked	


// READING MUSIC FILE
/****************************/
// - header information of 16-bit-wav-file 
typedef struct
{  
    unsigned long ulSampleRate;			// Sample rate in bytes per second.
   
    unsigned long ulAvgByteRate;		// The average byte rate for the wav file.
 
    unsigned long ulDataSize;			// The size of the wav data in the file.
   
    unsigned short usBitsPerSample;		// The number of bits per sample.
   
    unsigned short usFormat;			// The wav file format.
    
    unsigned short usNumChannels;		// The number of audio channels.
}
tWaveHeader;

static tWaveHeader g_sWaveHeader;

FILE *Fptr;
char psFileObject[100] = "M:SILENCE.WAV";  
short g_sSelected;						// store the select file in the wave-file-liste
static unsigned long g_ulBytesPlayed;	// State information for keep track of time.
static unsigned short g_usCount;



// BUFFER MANAGEMENT
/****************************/
#define PATH_BUF_SIZE   		80
#define CMD_BUF_SIZE    		64
#define AUDIO_BUFFER_SIZE       4096 	//(4096 / 2) = Size 1 Buffer
#define BUFFER_BOTTOM_EMPTY     0x00000001
#define BUFFER_TOP_EMPTY        0x00000002

static volatile unsigned long g_ulFlags;
static unsigned char g_pucBuffer[AUDIO_BUFFER_SIZE];
unsigned long g_ulMaxBufferSize;
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";
static char g_cCmdBuf[CMD_BUF_SIZE];



// WIDGET DEFINITION
/******************************/

// Globales variables for widget button
char g_sPlayText[5] = "Play";
char g_sStopText[5] = "Stop";

// string for info-widget
char g_pcTime[40]="";
char g_pcSamples[10]="";
char g_pcName[20]="";

// Global variables for widget-type
tPushButtonWidget g_sButtonStart;
tPushButtonWidget g_sButtonStop;
tCanvasWidget g_sWaveInfoTime;
tCanvasWidget g_sWaveInfoSample;
tCanvasWidget g_sWaveInfoName;
tListBoxWidget g_sDirList;

// Variables for rest-time-calculation
static unsigned long g_ulBytesRemaining;
static unsigned long g_ulBytesRemaining_old = 1000000000; 
static unsigned long one_sec = 0;						
static unsigned short g_usMinutes;
static unsigned short g_usSeconds;

int ul_minutes = 0;
int ul_seconds = 0;
int time_left = 0;
int time_total = 0;
int time_gone = 0;

// forward declaration
void UpdateTimeInfo(void);

/************************************************************************************************/
//
// FUNCTIONS WIDGETS
//
/************************************************************************************************/

// Callback-function for start-button
void on_button_play(tWidget *pWidget)
{
    start_flag = 1;	
	stop_flag = 0;
	
}

// Callback-function for stop-button
void on_button_stop(tWidget *pWidget)
{					
    stop_flag = 1;
	start_flag = 0;
}	

// Callbackfunction for widget slider
// Changes volume
void OnSliderChange (tWidget *pWidget, long lValue)
{
    SoundVolumeSet(lValue);
}

// Callbackfunction for info-widget
void UpdateFileInfo(void)
{
	//name
	usprintf(g_pcName,g_pcFilenames[g_sSelected]);  

    // type of channel
	if(g_sWaveHeader.usNumChannels == 1)
		{
		strcat(g_pcSamples,"Mono     ");
	}
	else
	{
		strcat(g_pcSamples,"Stereo   ");
	}
    
    // samplerate
	// usprintf(g_pcSamples,"%d Hz %d bit ", g_sWaveHeader.ulSampleRate,g_sWaveHeader.usBitsPerSample);
    
	WidgetPaint((tWidget *)&g_sWaveInfoTime);
	WidgetPaint((tWidget *)&g_sWaveInfoSample);
	WidgetPaint((tWidget *)&g_sWaveInfoName);
	
	UpdateTimeInfo();
}

// callbackfunction for list-widget
void OnListBoxChange (tWidget *pWidget, short usSelected)
{
    g_sSelected = ListBoxSelectionGet(&g_sDirList);
	// strcpy((char*)(&psFileObject), (char *)(&g_pcFilenames[g_sSelected]) );  
    stop_flag = 1;
    UpdateFileInfo();
}

// Callbackfunction for wave-file-time-info-widget
void UpdateTimeInfo(void)
{ 
	// calculation time
	one_sec = g_sWaveHeader.ulSampleRate * g_sWaveHeader.usBitsPerSample / 8;  // in Bytes
	time_left = g_ulBytesRemaining / one_sec / 2;		// 2 Channels
	time_total = g_usMinutes * 60 + g_usSeconds;		// in usec
	time_gone = time_total - time_left;
	ul_seconds = time_gone % 60;
	ul_minutes = time_gone / 60;  
	usprintf(g_pcTime,"%2d:%02d/%d:%02d", ul_minutes, ul_seconds, g_usMinutes, g_usSeconds);
	
	// output calcuation
	WidgetPaint( (tWidget *)&g_sWaveInfoTime );
}



// WIDGET INITIALISATION
/******************************/

// HEADER from ROOT
Canvas(g_sHeading, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkSlateGray, ClrWhite, ClrWhite, g_pFontCm20, "Audio-Player ", 0, 0);

// BUTTON	
// - Background
Canvas(g_sStartBackground, WIDGET_ROOT, 0, &g_sButtonStart,
       &g_sKitronix320x240x16_SSD2119, 190, 200, 90, 30,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20,"", 0, 0);
// - Start       
RectangularButton(g_sButtonStart, &g_sStartBackground, &g_sButtonStop, 0,
       &g_sKitronix320x240x16_SSD2119, 90, 200, 90, 30,
       (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT | PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
       ClrDarkGreen, ClrBlue, ClrWhite, ClrWhite, g_pFontCm20, g_sPlayText,0,0,0,0,on_button_play);
// - Stop
RectangularButton(g_sButtonStop, &g_sStartBackground, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 190, 200, 90, 30,
       (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT | PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
       ClrDarkRed, ClrBlue, ClrWhite, ClrWhite, g_pFontCm20, g_sStopText,0,0,0,0,on_button_stop);

// SLIDER for changing volume
Slider(g_sSlider, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 290, 30, 30, 180, 0, 100,
       INITIAL_VOLUME_PERCENT,(SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE | SL_STYLE_VERTICAL),
       ClrGray, ClrBlack, ClrWhite, ClrWhite, ClrWhite, 0, 0, 0, 0, OnSliderChange);

// LIST OF MUSIC-FILES
ListBox(g_sDirList, WIDGET_ROOT, 0, 0,
        &g_sKitronix320x240x16_SSD2119, 0, 30, 125, 140,
        LISTBOX_STYLE_OUTLINE, ClrBlack, ClrDarkBlue, ClrSilver, ClrWhite, ClrWhite, g_pFontCmss12, wave_pointer, NUM_LIST_STRINGS, 0, OnListBoxChange);

// INFO ABOUT WAVE-FILE-STATUS
// - Background
Canvas(g_sWaveInfoBackground, WIDGET_ROOT, 0, &g_sWaveInfoSample,
       &g_sKitronix320x240x16_SSD2119, 130, 30, 155, 80,
       (CANVAS_STYLE_OUTLINE | CANVAS_STYLE_FILL),
       ClrBlack, ClrWhite, ClrWhite, g_pFontCm12,0, 0, 0);
// - Sample         
Canvas(g_sWaveInfoSample, &g_sWaveInfoBackground, &g_sWaveInfoName, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 90, 140, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT | CANVAS_STYLE_TEXT_OPAQUE),
       ClrBlack, ClrWhite, ClrWhite, g_pFontFixed6x8,g_pcSamples, 0, 0);
// - Name         
Canvas(g_sWaveInfoName, &g_sWaveInfoBackground, &g_sWaveInfoTime, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 50, 140, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT | CANVAS_STYLE_TEXT_OPAQUE),
       ClrBlack, ClrWhite, ClrWhite, g_pFontFixed6x8,g_pcName, 0, 0);
// - Time        
Canvas(g_sWaveInfoTime, &g_sWaveInfoBackground, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 140, 70, 140, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT | CANVAS_STYLE_TEXT_OPAQUE),
       ClrBlack, ClrWhite, ClrWhite, g_pFontFixed6x8,g_pcTime, 0, 0);



/************************************************************************************************/
//
// FUNCTIONS GENERAL
//
/************************************************************************************************/

static int PopulateFileListBox(tBoolean bRepaint)
{
     unsigned long ulItemCount;
     FINFO info;

     // Empty the list box on the display.
     ListBoxClear(&g_sDirList);

    //************************************************************************
    // Make sure the list box will be redrawn next time the message queue
    // is processed.
    //************************************************************************
     if(bRepaint)
     {
         WidgetPaint((tWidget *)&g_sDirList);
     }

     ulItemCount = 0;

    // Enter loop to enumerate through all directory entries.
    // Read an entry from the directory.
    while (ffind ("M:*.*", &info) == 0)
	
				 
       if(ulItemCount < NUM_LIST_STRINGS)
       {
			strcpy( (char *)(&g_pcFilenames[ulItemCount]), (const char *)(info.name) );  
			ListBoxTextAdd(&g_sDirList, g_pcFilenames[ulItemCount]); 					
            ulItemCount++;
	  }	
	  // WidgetPaint((tWidget *)&g_sDirList);
			
     return(0);
}


// Help lists all available functions  
int Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    // Print some header text.
    UARTprintf("\nAvailable commands\n");
    UARTprintf("------------------\n");

    // Point at the beginning of the command table.
    // pEntry = &g_sCmdTable[0];

    // Enter a loop to read each entry from the command table.  The
    // end of the table has been reached when the command name is NULL.
    while(pEntry->pcCmd)
    {

        // Print the command name and the brief description.
        UARTprintf("%s%s\n", pEntry->pcCmd, pEntry->pcHelp);


        // Advance to the next entry in the table.
        pEntry++;


        // Wiat for the UART to catch up.
        UARTFlushTx(false);
    }

    return(0);
}




// Handler for bufffers being released.
void BufferCallback(void *pvBuffer, unsigned long ulEvent)
{
    if(ulEvent & BUFFER_EVENT_FREE)
    {
        if(pvBuffer == g_pucBuffer)
        {
            // Flag if the first half is free.
            g_ulFlags |= BUFFER_BOTTOM_EMPTY;
        }
        else
        {
            // Flag if the second half is free.
            g_ulFlags |= BUFFER_TOP_EMPTY;
        }

        // Update the byte count.
        g_ulBytesPlayed += AUDIO_BUFFER_SIZE >> 1;
    }
}


// This function can be used to test if a file is a wav file or not and will
// also return the wav file header information in the pWaveHeader structure.
// If the file is a wav file then the psFileObject pointer will contain an
// open file pointer to the wave file ready to be passed into the WavePlay()
// function.
FRESULT WaveOpen(const char *pcFileName, tWaveHeader *pWaveHeader)
{
	unsigned long *pulBuffer;
    unsigned short *pusBuffer;
    unsigned long ulChunkSize;
    unsigned long ulBytesPerSample;

    pulBuffer = (unsigned long *)g_pucBuffer;
    pusBuffer = (unsigned short *)g_pucBuffer;

	Fptr = fopen(pcFileName, "r");
	if(Fptr == NULL)
    {
        return(FR_INVALID_OBJECT);
    }
		
    // Read the first 12 bytes.
	fread (&g_pucBuffer[0], 1, 12, Fptr);    		

    // Look for RIFF tag.
    if((pulBuffer[0] != RIFF_CHUNK_ID_RIFF) || (pulBuffer[2] != RIFF_TAG_WAVE))
    {
        fclose(Fptr);
        return(FR_INVALID_OBJECT);
    }

    // Read the next chunk header. 
    fread(&g_pucBuffer[0], 1, 8, Fptr);    
    
    if(pulBuffer[0] != RIFF_CHUNK_ID_FMT)
    {
        fclose(Fptr);
        return(FR_INVALID_OBJECT);
    }

    // Read the format chunk size.
    ulChunkSize = pulBuffer[1];

    if(ulChunkSize > 16)
    {
        fclose(Fptr);
        return(FR_INVALID_OBJECT);
    }

    // Read the next chunk header.				
    fread(&g_pucBuffer[0], 1, ulChunkSize, Fptr);    

    pWaveHeader->usFormat = pusBuffer[0];
    pWaveHeader->usNumChannels =  pusBuffer[1];
    pWaveHeader->ulSampleRate = pulBuffer[1];
    pWaveHeader->ulAvgByteRate = pulBuffer[2];
    pWaveHeader->usBitsPerSample = pusBuffer[7];

    // Reset the byte count.
    g_ulBytesPlayed = 0;

    // Calculate the Maximum buffer size based on format.  There can only be
    // 1024 samples per ping pong buffer due to uDMA.
    ulBytesPerSample = (pWaveHeader->usBitsPerSample *
                        pWaveHeader->usNumChannels) >> 3;

    if(((AUDIO_BUFFER_SIZE >> 1) / ulBytesPerSample) > 1024)
    {
        // The maximum number of DMA transfers was more than 1024 so limit
        // it to 1024 transfers.
        g_ulMaxBufferSize = 1024 * ulBytesPerSample;
    }
    else
    {
        // The maximum number of DMA transfers was not more than 1024.
        g_ulMaxBufferSize = AUDIO_BUFFER_SIZE >> 1;
    }

    // Only mono and stereo supported.
    if(pWaveHeader->usNumChannels > 2)
    {
        fclose(Fptr);
        return(FR_INVALID_OBJECT);
    }

    // Read the next chunk header.				
    fread(&g_pucBuffer[0], 1, 8, Fptr);    

    if(pulBuffer[0] != RIFF_CHUNK_ID_DATA)
    {
        fclose(Fptr);
        return(FR_INVALID_OBJECT);
    }

    // Save the size of the data.
    pWaveHeader->ulDataSize = pulBuffer[1];

    g_usSeconds = pWaveHeader->ulDataSize/pWaveHeader->ulAvgByteRate;
    g_usMinutes = g_usSeconds/60;
    g_usSeconds -= g_usMinutes*60;

    // Set the number of data bytes in the file.
    g_ulBytesRemaining = pWaveHeader->ulDataSize;

    // Adjust the average bit rate for 8 bit mono files.
    if((pWaveHeader->usNumChannels == 1) && (pWaveHeader->usBitsPerSample == 8))
    {
        pWaveHeader->ulAvgByteRate <<=1;
    }

    // Set the format of the playback in the sound driver.
    SoundSetFormat(pWaveHeader->ulSampleRate, pWaveHeader->usBitsPerSample,
                   pWaveHeader->usNumChannels);
	
    return(FR_OK);
}


// Convert an 8 bit unsigned buffer to 8 bit signed buffer in place so that it
// can be passed into the i2s playback.
void Convert8Bit(unsigned char *pucBuffer, unsigned long ulSize)
{
    unsigned long ulIdx;

    for(ulIdx = 0; ulIdx < ulSize; ulIdx++)
    {
        // In place conversion of 8 bit unsigned to 8 bit signed.
        *pucBuffer = ((short)(*pucBuffer)) - 128;
        pucBuffer++;
    }
}



// This function will handle reading the correct amount from the wav file and
// will also handle converting 8 bit unsigned to 8 bit signed if necessary.

unsigned short WaveRead(const char *pcFileName, tWaveHeader *pWaveHeader, unsigned char *pucBuffer)
{
    unsigned long ulBytesToRead;	 
    unsigned short usCount;

    // Either read a half buffer or just the bytes remaining if we are at the
    // end of the file.
    if(g_ulBytesRemaining < g_ulMaxBufferSize)
    {
        ulBytesToRead = g_ulBytesRemaining;
    }
    else
    {
        ulBytesToRead = g_ulMaxBufferSize;
    }

    // Calculate Number of Bytes which were read out and
	// Read out Sound Data from the File		
	usCount = fread (&pucBuffer[0], 1, ulBytesToRead, Fptr);     
	if ( usCount == 0 ) 
	{
			return 0;
	 }
	
    // Decrement the number of data bytes remaining to be read.
    g_ulBytesRemaining -= usCount;

    // Need to convert the audio from unsigned to signed if 8 bit
    // audio is used.
    if(pWaveHeader->usBitsPerSample == 8)
    {
        Convert8Bit(pucBuffer, usCount);
    }
    return(usCount);
}

//******************************************************************************
//
// This will play the file passed in via the psFileObject parameter based on
// the format passed in the pWaveHeader structure.  The WaveOpen() function
// can be used to properly fill the pWaveHeader and psFileObject structures.
//
//******************************************************************************
unsigned long WavePlay(const char *pcFileName, tWaveHeader *pWaveHeader)
{
        // Must disable I2S interrupts during this time to prevent state
        // problems.
        IntDisable(INT_I2S0);

        // If the refill flag gets cleared then fill the requested side of the
        // buffer.
        if(g_ulFlags & BUFFER_BOTTOM_EMPTY)
        {

            // Read out the next buffer worth of data.
            g_usCount = WaveRead(psFileObject, pWaveHeader, g_pucBuffer);


            // Start the playback for a new buffer.
            SoundBufferPlay(g_pucBuffer, g_usCount, BufferCallback);


            // Bottom half of the buffer is now not empty.
            g_ulFlags &= ~BUFFER_BOTTOM_EMPTY;
        }

        if(g_ulFlags & BUFFER_TOP_EMPTY)
        {

            // Read out the next buffer worth of data.
            g_usCount = WaveRead(psFileObject, pWaveHeader,
                               &g_pucBuffer[AUDIO_BUFFER_SIZE >> 1]);


            // Start the playback for a new buffer.
            SoundBufferPlay(&g_pucBuffer[AUDIO_BUFFER_SIZE >> 1],
                            g_usCount, BufferCallback);


            // Top half of the buffer is now not empty.
            g_ulFlags &= ~BUFFER_TOP_EMPTY;
			
        }

        // Audio playback is done once the count is below a full buffer.
        if((g_usCount < g_ulMaxBufferSize) || (g_ulBytesRemaining == 0))
        {

            // Wait for the buffer to empty.
            while(g_ulFlags != (BUFFER_TOP_EMPTY | BUFFER_BOTTOM_EMPTY))
            {
            }
						fclose(Fptr);
        }

        // Must disable I2S interrupts during this time to prevent state
        // problems.
        IntEnable(INT_I2S0);

    return(0);
}



//*****************************************************************************
//
// TASKS
//
//*****************************************************************************


__task void UART_task(void)
{
    int nStatus;	
    while(1)
	{
        UARTprintf("\n%s> ", g_cCwdBuf);

        // Get a line of text from the user.
        UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));

        // Pass the line from the user to the command processor.
        // It will be parsed and valid commands executed.
        nStatus = CmdLineProcess(g_cCmdBuf);

        // Handle the case of bad command.
        if(nStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        // Handle the case of too many arguments.
        else if(nStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }
		os_dly_wait(1);
    }
}


// state machine reactiong on
// - commands from console 	(play, stop)
// - touchscreen button play and stop
__task void audioplay_task(void)
{ 
	// taskID_3 = os_tsk_create(UART_task,0x10);
    while(1)
	{
        
        switch(player_state)
        {
            case stop:				
                if( start_flag == 1 )
                {
                    player_state = play;
					
                    WaveOpen(g_pcFilenames[g_sSelected],&g_sWaveHeader);  
					// WaveOpen(psFileObject, &g_sWaveHeader);
                    g_ulFlags = BUFFER_BOTTOM_EMPTY|BUFFER_TOP_EMPTY;
                    
					//WavePlay(g_pcFilenames[g_sSelected],&g_sWaveHeader);		
					WavePlay(psFileObject, &g_sWaveHeader);					
                    start_flag = 0;
                }
                else
                {
                    player_state = stop;
                }
				break;
				
            case play:
                if(stop_flag == 1)
                {
					player_state = stop;
                    fclose(Fptr);
                    stop_flag = 0;
                }
                else
                {
                    player_state = play;
					//WavePlay(psFileObject, &g_sWaveHeader);
                    WavePlay(g_pcFilenames[g_sSelected],&g_sWaveHeader); 
                }
            break;
        }
        
        // time for widget: read button
        os_dly_wait(4);
    }   
}


__task void widget_task(void)
{
    TouchScreenInit();
    TouchScreenCallbackSet(WidgetPointerMessage);
    PopulateFileListBox(1);

	// Add widgets
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sStartBackground);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sSlider);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sDirList);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sWaveInfoBackground);			
	
    // Display widgets
    WidgetPaint(WIDGET_ROOT);
	
	// Create next task
    taskID_2 = os_tsk_create(audioplay_task,0x20);
	
    while(1)
	{		
        WidgetMessageQueueProcess();
		
		// calculation rest-time
        if( (g_ulBytesRemaining_old - g_ulBytesRemaining )> one_sec)
		{
			UpdateTimeInfo();
			g_ulBytesRemaining_old = g_ulBytesRemaining;
        }   
		
    }
}

__task void rtos_init_task(void)
{
    taskID_1 = os_tsk_create(widget_task,0x10);
    os_tsk_delete_self();
}

//*****************************************************************************
//
// BEGIN MAIN
//
//*****************************************************************************

int main(void) 
{
  sysInit_hal();
  ledInit_hal();
  finit();
  PinoutSet();  
  LCDPinoutSet();
  Kitronix320x240x16_SSD2119Init();
  SoundInit(0);								// Configure the I2S peripheral.
  SoundVolumeSet(INITIAL_VOLUME_PERCENT);
  os_sys_init(rtos_init_task);
	
    while(1)
    { 
    }
}


