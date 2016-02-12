#ifndef HG_APP_MAIN
#define HG_APP_MAIN

#include "stdint.h"

// Value definitions

#define UART_CMDBUFFERSIZE_APP          100

#define UART_STATE_RESTART_APP          0
#define UART_STATE_PENDING_APP          1
#define UART_STATE_COMPLETE_APP         2

// Structures

typedef struct {
  // Uart
  uint8_t uartState;
  uint8_t uartUsedSize;
  uint8_t *uartCmdLine;
} Globals_App;


/* File function return code (FRESULT) */
typedef enum {
    FR_OK = 0,            /* 0 */
    FR_NOT_READY,        /* 1 */
    FR_NO_FILE,            /* 2 */
    FR_NO_PATH,            /* 3 */
    FR_INVALID_NAME,    /* 4 */
    FR_INVALID_DRIVE,    /* 5 */
    FR_DENIED,            /* 6 */
    FR_EXIST,            /* 7 */
    FR_RW_ERROR,        /* 8 */
    FR_WRITE_PROTECTED,    /* 9 */
    FR_NOT_ENABLED,        /* 10 */
    FR_NO_FILESYSTEM,    /* 11 */
    FR_INVALID_OBJECT,    /* 12 */
    FR_MKFS_ABORTED        /* 13 */
} FRESULT;

// Prototypes

void cmdLed( uint8_t *args );
int Cmd_pwd(int argc, char *argv[]);
int Cmd_write(int argc, char *argv[]);
int Cmd_dummy(int argc, char *argv[]);
int Cmd_ls(int argc, char *argv[]);
int Cmd_cat(int argc, char *argv[]);
int Cmd_play(int argc, char *argv[]);
int Cmd_stop(int argc, char *argv[]);
void write_file (void);

#endif
