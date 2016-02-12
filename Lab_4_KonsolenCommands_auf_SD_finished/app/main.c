/***********************************************************************************/
// Lab 4: File System
// Implementation of a Kommando Interpreter from the Console
// Comand Table accessed by the pointer tCmdLineEntry
// The Comand Table is a struct! Each line has the type comand, function, help
/***********************************************************************************/
#include <RTL.h>
#include <stdio.h>
#include <string.h>
#include "inc/hw_types.h"
#include "main.h"
#include "hal_led.h"
#include "hal_system.h"
#include "uartstdio.h"
#include "ustdlib.h"
#include "cmdline.h"

#define PATH_BUF_SIZE   80		// Buffers which saves the path 
								// or temporary the data from SD-Card
#define CMD_BUF_SIZE    64		// Buffer which holds the command line.
								// Initially it is root ("/").

static char g_cCwdBuf[PATH_BUF_SIZE] = "/";
static char g_cCmdBuf[CMD_BUF_SIZE];


/**********************************************************************************************/
// Funcitions
/***********************************************************************************************/


// Function implements the "help" command.  It prints a simple list
// of the available commands with a brief description.
int Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;          // Point at the beginning of the command table

    UARTprintf("\nAvailable commands\n");
    UARTprintf("------------------\n");
    
    pEntry = &g_sCmdTable[0];       // The command table is a struct with 3 subtypes: 
                                       // pcCmd (Komando Konsole	pfnCmd (Function)	pcHelp

    // loop check, if command comes from
    // end when the command is NULL
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

// Formats Memory
// The Char auf the drive must be given 
// in Capslock-Letter with : after 1 space
int Cmd_format(int argc, char *argv[])
{
    char *drive;
    drive = argv[1];
    
   if (fformat(drive) != 0){
       UARTprintf("\nSorry.There is a format failure. \n");
   } else{
        UARTprintf("\nThe Drive %s is formated.\n",drive);
   }
   return(0);
}

int Cmd_check(int argc, char *argv[])
{
   if (fcheck("R:") != 0){
       UARTprintf("\nThe check of Ram-formating is failed.\n");
       return(CMDLINE_BAD_CMD);
   } else{
        UARTprintf("\nThe formating of the RAM Drive R is checked.\n");
        return(0);
   }
}

// Generates 4 Files in the choosen drive
int Cmd_write(int argc, char *argv[])
{
    char string[11];
    char *drive, *file;
    FILE *Fptr;
    
    drive = argv[1];                // the choosen drive
    
    //File Nr.1
    file = strcat(drive, "datei1.txt");   
    Fptr = fopen (file, "w");
    if(Fptr != 0){
        fwrite("LiebeDatei", 8, 10, Fptr);
        fclose(Fptr);
        UARTprintf("\nDie datei1.txt ist geschrieben. ");
    } else {
        UARTprintf("\nDatei 1 konnte nicht erzeugt werden\n");
    }
    fclose(Fptr);
    
    Fptr = fopen(file,"r");
    if(Fptr != 0){
        fgets(string, 11, Fptr);
        UARTprintf("\t In datei1.txt steht: %s\n",string);
    } else {
        UARTprintf("\nFile 1 not found!!!\n");
    }
    fclose(Fptr);
    
    //File Nr.2
    file = strcat(drive, "datei2.txt");   
    Fptr = fopen (file, "w");
    if(Fptr != 0){
        fwrite("Das ist", 8, 10, Fptr);
        fclose(Fptr);
        UARTprintf("\nDie datei2.txt ist geschrieben.");
    } else {
        UARTprintf("\nDatei 2 konnte nicht erzeugt werden\n");
    }
    fclose(Fptr);
    
    Fptr = fopen(file,"r");
    if(Fptr != 0){
        fgets(string, 11, Fptr);
        UARTprintf("\t\t In datei2.txt steht: %s\n",string);
    } else {
        UARTprintf("\nFile 2 not found!!!\n");
    }
    fclose(Fptr);
    
       
    //File Nr.3
    file = strcat(drive, "datei3.txt");   
    Fptr = fopen (file, "w");
    if(Fptr != 0){
        fwrite("gut!!", 8, 10, Fptr);
        fclose(Fptr);
        UARTprintf("\nDie datei3.txt ist geschrieben.");
    } else {
        UARTprintf("\nDatei 3 konnte nicht erzeugt werden\n");
    }
    fclose(Fptr);
    
    Fptr = fopen(file,"r");
    if(Fptr != 0){
        fgets(string, 11, Fptr);
        UARTprintf("\t\t In datei3.txt steht: %s\n",string);
    } else {
        UARTprintf("\nFile 3 not found!!!\n");
    }
    fclose(Fptr);
    
   // File Nr.4
    file = strcat(drive, "datei4.txt");
    Fptr = fopen (file, "w");
    if(Fptr != 0){
        fwrite("Gruss", 8, 10, Fptr);
        fclose(Fptr);
        UARTprintf("\nDie datei4.txt ist geschrieben.");
    } else {
        UARTprintf("\nDie datei4.txt nicht geschrieben.");
    }
    fclose(Fptr);
    
    Fptr = fopen(file,"r");
    if(Fptr != 0){
        fgets(string, 11, Fptr);
        UARTprintf("\t\t In datei4.txt steht: %s\n",string);
    } else {
        UARTprintf("\t\t In datei4.txt steht: nichts\n");
    }
    fclose(Fptr);
    
	return 0;
}

int Cmd_list(int argc, char *argv[])
{
    FINFO info;
    char *drive, *search_until;
   
    info.fileID = 0;
    drive = argv[1];
    search_until = strcat(drive, "*.*");
    
    while (ffind(search_until, &info) == 0) {
        // Search in named drive for all files
        UARTprintf("\nName: %s \t %5d Bytes \tID: %04d \n", info.name, info.size, info.fileID); 
    }
    return(0);
}


// shwos the content of the choosen file in the choosen drive
int Cmd_cat(int argc, char *argv[])
{
    FILE *Fptr;
    char *filename, *string;
    
    filename = argv[1];
    Fptr = fopen(filename,"r");
    
    if(Fptr != 0){
        UARTprintf("\nRead %s:\n", filename);
        fgets(string, 11, Fptr);
        UARTprintf("im File steht: %s\n",string);
    } else {
        UARTprintf("\n%s not found!!!\n", filename);
    }
    fclose(Fptr);
    return(0);
}










int Cmd_rename(int argc, char *argv[])
{
    char *fileold, *filenew;
    fileold = argv[1];
    filenew = argv[2];
    UARTprintf("File %s renamed to %s.\n",fileold,filenew);
    frename(fileold,filenew);
   
    return(0);
}

int Cmd_rm (int argc, char *argv[])
{
    char *filename;
    filename = argv[1];
    
    UARTprintf("File %s removed\n",filename);
    fdelete(filename);
   
    return(0);
}


// Tabelle definiert über struct: Kommando, Funktion, Beschreibung
// Funktions-Parameter:    int argc => Zählt wie viele Argumente, 
//                       *char ArgV => Liest chars bis  zu einem Leerschlag
// 							argV[1] => ist das zweite Wort (nach einem Leerschlag)
tCmdLineEntry g_sCmdTable[] =
{
    { "help",   Cmd_help,  "\t: Display list of commands" },
    { "format", Cmd_format,"\t: Formats drive <Letter>: \n\t  The formating process takes some time. \n\t  Please wait." },
    { "check_R", Cmd_check, "\t: Checks RAM. (Only command is needed)"},
	{ "write",  Cmd_write, "\t: Fill drive with 4 files 'dateix.txt'. \n\t  - Drive <letter>: is first argumentd. \n\t  - The following arguments are contents. \n\t  - The max. is 4 files. " },
    { "ls",     Cmd_list,      "\t: Display list of files in the drive. \n\t  - Drive <letter>: is needed." },

    { "cat",    Cmd_cat,      "\t: Show contents of the file. \n\t  - Drive <letter>: filename <string.txt>." },
//  { "h",      Cmd_help,       "  : alias for help" },
//  { "?",      Cmd_help,       "  : alias for help" },
//  { "chdir",  Cmd_dummy,      "  : Change directory" },
//  { "cd",     Cmd_dummy,      "  : alias for chdir" },
//  { "pwd",    Cmd_dummy,      "  : Show current working directory" },
//	{ "dummy",  Cmd_dummy,      "  : Fill R: with Files" },
//  { "formatR", Cmd_format,      "  : Format Drive R:" },
//  { "rm", Cmd_rm,     "  : remove file"},
//  { "rename", Cmd_rename, "  rename file"},
    {0,0,0}
};




// This function implements the "dummy" command.  
int Cmd_dummy(int argc, char *argv[])
{

	
 UARTprintf("\nEnter your code here\n");
	
    return(0);
}




//*****************************************************************************
//
// BEGIN MAIN
//
//*****************************************************************************


int main(void) 
{
	int nStatus;
	
    sysInit_hal();
    ledInit_hal();
    finit("M:");
	if (finit("M:") != 0) {
        UARTprintf ("File System Initialization failed.\n");
    }
	UARTprintf("\n\nSD Card Example Program\n");
    UARTprintf("Type \'help\' for help.\n");
	
    //
    // Enter an (almost) infinite loop for reading and processing commands from
    //the user.
    //
    while(1)
    {
        //
        // Print a prompt to the console.  Show the CWD.
        //
        UARTprintf("\n%s> ", g_cCwdBuf);

        //
        // Get a line of text from the user.
        //
        UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));

        //
        // Pass the line from the user to the command processor.
        // It will be parsed and valid commands executed.
        //
        nStatus = CmdLineProcess(g_cCmdBuf);

        //
        // Handle the case of bad command.
        //
        if(nStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        //
        // Handle the case of too many arguments.
        //
        else if(nStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }

        
    }
}


