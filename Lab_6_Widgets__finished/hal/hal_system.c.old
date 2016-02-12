
#include "hal_system.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
// from here new for lab
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "inc/hw_memmap.h"
#include "uartstdio.h"
#include "driverlib/uart.h"



uint8_t sysInit_hal( void ) {

  // Configure 66.6.. MHz clock
  SysCtlClockSet( SYSCTL_SYSDIV_3 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ );
	
	IntMasterEnable();
    //
    // Set GPIO A0 and A1 as UART.
    //
	
	SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOA );            // Enable GPIO Port A peripheral ( enable clock for unit )
  SysCtlPeripheralEnable( SYSCTL_PERIPH_UART0 );            // Enable UART peripheral ( enable clock for unit )
  UARTEnable( UART0_BASE );     
	
	
	
	
	// Set GPIO A0 and A1 as UART pins.
   //GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	GPIOPinTypeUART( GPIO_PORTA_BASE, GPIO_PIN_0 );          // GPIO RX Pin Funktion wählen
  GPIOPinTypeUART( GPIO_PORTA_BASE, GPIO_PIN_1 );   
	
	
	
    //GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART as a console for text I/O.
    //
    UARTStdioInit(0);

  return 0;

  
}

