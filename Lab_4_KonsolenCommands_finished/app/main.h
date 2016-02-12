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

// Prototypes

void cmdLed( uint8_t *args );
int Cmd_pwd(int argc, char *argv[]);
int Cmd_write(int argc, char *argv[]);
int Cmd_dummy(int argc, char *argv[]);
int Cmd_ls(int argc, char *argv[]);
void write_file (void);

#endif
