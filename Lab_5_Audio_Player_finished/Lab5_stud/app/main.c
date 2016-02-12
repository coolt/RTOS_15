#include <RTL.h>
#include <stdio.h>
#include "inc/hw_types.h"
#include "main.h"
#include "hal_led.h"
#include "hal_system.h"
#include "uartstdio.h"
#include "ustdlib.h"
#include "cmdline.h"
#include "sound.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "string.h"

#define RIFF_CHUNK_ID_RIFF      0x46464952
#define RIFF_CHUNK_ID_FMT       0x20746d66
#define RIFF_CHUNK_ID_DATA      0x61746164
#define RIFF_TAG_WAVE           0x45564157
#define RIFF_FORMAT_UNKNOWN     0x0000
#define RIFF_FORMAT_PCM         0x0001
#define RIFF_FORMAT_MSADPCM     0x0002
#define RIFF_FORMAT_IMAADPCM    0x0011

#define INITIAL_VOLUME_PERCENT 40


OS_TID taskID_1, taskID_2;


// STATE-MACHINE AUDIO PLAYER
/********************************/
typedef enum								
{	
	stop,
	play
	
} STATE_AUDIO;

STATE_AUDIO player_state = stop;			// Initial-State State machine
char start_flag = 0;						// State machine is cotrolled by console
char stop_flag  = 0;


// BUFFER MANAGEMENT
/********************************/
#define PATH_BUF_SIZE   80					// Defines the size of the buffers that hold the path, or temporary
											// data from the SD card. 
#define CMD_BUF_SIZE    64					// Defines the size of the buffer that holds the command line.
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";	// Initial-state of the path ("/").
static char g_cCmdBuf[CMD_BUF_SIZE];		// The buffer that holds the command line.

#define AUDIO_BUFFER_SIZE       4096
static unsigned char g_pucBuffer[AUDIO_BUFFER_SIZE];
unsigned long g_ulMaxBufferSize;

#define BUFFER_BOTTOM_EMPTY     0x00000001	// Flags used in the g_ulFlags global variable.
#define BUFFER_TOP_EMPTY        0x00000002
static volatile unsigned long g_ulFlags;




// WAVE-FILE MANAGEMENT
/********************************/

typedef struct								// WAV-Header
{
    unsigned long ulSampleRate;				// Sample rate in bytes per second.    
    unsigned long ulAvgByteRate;			// The average byte rate for the wav file.
    unsigned long ulDataSize;				// The size of the wav data in the file  
    unsigned short usBitsPerSample;			// The number of bits per sample.
    unsigned short usFormat;			    // The wav file format.
    unsigned short usNumChannels;		    // The number of audio channels.
}
tWaveHeader;
static tWaveHeader g_sWaveHeader;

static unsigned short g_usCount;
FILE *Fptr = 0;
char psFileObject[100];						// fix adress for storing the temp. path
											// feeded by the keyboard by command start playing
static unsigned long g_ulBytesPlayed;		// State information for keep track of time.

// for calculation time of played music
static unsigned long g_ulBytesRemaining;
static unsigned short g_usMinutes;
static unsigned short g_usSeconds;


/*****************************************************************************************************/
// FUNCTIONS
/*****************************************************************************************************/

// the "help" command lists alle available functions
int Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    UARTprintf("\nAvailable commands\n");
    UARTprintf("------------------\n");

    // Point at the beginning of the command table.
    pEntry = &g_sCmdTable[0];

    while(pEntry->pcCmd)
    {
        // Print the command name and the brief description.
        UARTprintf("%s%s\n", pEntry->pcCmd, pEntry->pcHelp);

        // Advance to the next entry in the table.
        pEntry++;

        // Wait for the UART to catch up.
        UARTFlushTx(false);
    }
    return(0);
}


// Table:
// command names, implementing functions, brief description.
tCmdLineEntry g_sCmdTable[] =
{
    { "help",   Cmd_help,      " : Display list of commands" },
    { "ls",     Cmd_ls,      "   : Display list of files" },
    { "cat",    Cmd_cat,      "  : Show contents of a text file" },
	{ "play",  Cmd_play,      "  : Start Sound. Write M:filename.WAV" },
	{ "stop",  Cmd_stop,      "  : Stop Sound" },
    { 0, 0, 0 }
};


// This function implements the "play" command. 
// The Directory must be named, otherwise the function does not recgnoize the path
int Cmd_play(int argc, char *argv[])
{   
	start_flag = 1;							// event-state for the state-machine
	stop_flag = 0;
	strcpy(psFileObject, argv[1]);			// String with Adresse is stored in FileObject
	UARTprintf("%s", psFileObject);
    return(0);
}



// This function implements the "stop" command.  
int Cmd_stop(int argc, char *argv[])
{
	stop_flag = 1;
	start_flag = 0;
    return(0);
}


// This function implements the "ls" command.  It opens the current
// directory and enumerates through the contents
int Cmd_ls(int argc, char *argv[])
{
FINFO info;
	info.fileID = 0;
	
	while (ffind ("M:*.*", &info) == 0) 
	{
		UARTprintf("\nName: %s %5d bytes ID: %04d", info.name, info.size, info.fileID);
	}
	if (info.fileID == 0) {
	UARTprintf("Empty Directory");
	}
    return(0);
}

// This function implements the "cat" command.
// The content of a file is printed
int Cmd_cat(int argc, char *argv[])
{
	FILE *Fptr;
	char c;
	if(argc == 2) {
		Fptr = fopen(argv[1],"r");
		if (Fptr == NULL)  {
			UARTprintf("\nFile doesn't exist!\n");
		}
		else  {
			UARTprintf("\n");
			while(!feof(Fptr)) {
				c = fgetc(Fptr);
				if(c != '\0') {
					UARTprintf("%c",c);
				}
			}
			UARTprintf("\n");
			fclose(Fptr);
		}
	}
	else {
		UARTprintf("\nArgument missing!\n");
	}
	return(0);
}


// Handler for bufffers being released.
/*****************************************/
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

// This will play the file passed in via the psFileObject parameter based on
// the format passed in the pWaveHeader structure.  The WaveOpen() function
// can be used to properly fill the pWaveHeader and psFileObject structures.
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
				UARTprintf("Endlose while?\n");
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
__task void task2_audio(void)
{	
	while(1)
	{
		switch(player_state)
		// initial states:
		// player_state = stop, 
		// stop_flag  = 0; start_flag = 0;	
		{
			case stop:				
				// from stop to start
				if( start_flag == 1 )
				{
					player_state = play;
					UARTprintf("Wechsel von Stop zu Play\n");
					
					WaveOpen(psFileObject, &g_sWaveHeader);					
					g_ulFlags = BUFFER_BOTTOM_EMPTY | BUFFER_TOP_EMPTY;
					
					WavePlay(psFileObject, &g_sWaveHeader);	
					start_flag = 0;
					
				}
				
				else
				{
					player_state = stop;
				}			
				break;
			case play:
				
				if( stop_flag == 1)
				{
					player_state = stop;
					UARTprintf("Wechsel von Play zu Stop\n");
					stop_flag = 0;
					fclose(Fptr);
				}
				else
				{
					player_state = play;
					WavePlay(psFileObject, &g_sWaveHeader);
				}
				break;			
		}
		os_dly_wait(4);
	}
}
__task void task1_uart(void)
{
	int nStatus;
	taskID_2 = os_tsk_create(task2_audio,0x20);
		
   UARTprintf("\n\nSD Card Example Program\n");
	while(1)
		{
        // Print a prompt to the console.  Show the CWD (Current Working Directory)
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
	}
}

// Initializing-Task
__task void rtos_init_task(void)
{
	// initialize task1
	taskID_1 = os_tsk_create(task1_uart,0x10);		
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
    SoundInit(0);
    SoundVolumeSet(INITIAL_VOLUME_PERCENT);
	os_sys_init(rtos_init_task); 

    while(1)
    { 
    }
}


