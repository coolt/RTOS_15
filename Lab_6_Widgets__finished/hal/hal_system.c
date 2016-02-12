
#include "hal_system.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
// from here new for lab4
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "inc/hw_memmap.h"
#include "uartstdio.h"
#include "driverlib/uart.h"
// from here new for lab5
#include "driverlib/udma.h"

#include "inc/hw_gpio.h"

// I2S Stuff

#define I2S0_SCLKTX_PERIPH      (SYSCTL_PERIPH_GPIOB)
#define I2S0_SCLKTX_PORT        (GPIO_PORTB_BASE)
#define I2S0_SCLKTX_PIN         (GPIO_PIN_6)

#define I2S0_MCLKTX_PERIPH      (SYSCTL_PERIPH_GPIOF)
#define I2S0_MCLKTX_PORT        (GPIO_PORTF_BASE)
#define I2S0_MCLKTX_PIN         (GPIO_PIN_1)

#define I2S0_LRCTX_PERIPH       (SYSCTL_PERIPH_GPIOE)
#define I2S0_LRCTX_PORT         (GPIO_PORTE_BASE)
#define I2S0_LRCTX_PIN          (GPIO_PIN_4)

#define I2S0_SDATX_PERIPH       (SYSCTL_PERIPH_GPIOE)
#define I2S0_SDATX_PORT         (GPIO_PORTE_BASE)
#define I2S0_SDATX_PIN          (GPIO_PIN_5)

#define GPIO_PCTL_PB6_I2S0TXSCK    0x09000000  // I2S0TXSCK on PB6
#define GPIO_PCTL_PF1_I2S0TXMCLK   0x00000080  // I2S0TXMCLK on PF1
#define GPIO_PCTL_PE4_I2S0TXWS     0x00090000  // I2S0TXWS on PE4
#define GPIO_PCTL_PE5_I2S0TXSD     0x00900000  // I2S0TXSD on PE5

//******************************************************************************
//
// The DMA control structure table.
//
//******************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable sDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sDMAControlTable, 1024)
tDMAControlTable sDMAControlTable[64];
#else
tDMAControlTable sDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

void PinoutSet( void ) {

	unsigned long pctl;

  //
  // Set_Pinout
  //
  SysCtlPeripheralEnable( I2S0_SCLKTX_PERIPH );
  SysCtlPeripheralEnable( I2S0_MCLKTX_PERIPH );
  SysCtlPeripheralEnable( I2S0_LRCTX_PERIPH );
  SysCtlPeripheralEnable( I2S0_SDATX_PERIPH );

  // I2S0TXSCK on PB6
  pctl = HWREG( GPIO_PORTB_BASE + GPIO_O_PCTL );
  pctl |= GPIO_PCTL_PB6_I2S0TXSCK;
  HWREG( GPIO_PORTB_BASE + GPIO_O_PCTL ) = pctl;

  // I2S0TXMCLK on PF1
  pctl = HWREG( GPIO_PORTF_BASE + GPIO_O_PCTL );
  pctl |= GPIO_PCTL_PF1_I2S0TXMCLK;
  HWREG( GPIO_PORTF_BASE + GPIO_O_PCTL ) = pctl;

  // I2S0TXWS and I2S0TXSD on PE4 and PE5
  pctl = HWREG( GPIO_PORTE_BASE + GPIO_O_PCTL );
  pctl |= ( GPIO_PCTL_PE4_I2S0TXWS | GPIO_PCTL_PE5_I2S0TXSD );
  HWREG( GPIO_PORTE_BASE + GPIO_O_PCTL ) = pctl;

}

uint8_t sysInit_hal( void ) {

  // Configure 66.6.. MHz clock
  SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ );
	
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
		
		
		//
    // Configure and enable uDMA
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    uDMAControlBaseSet(&sDMAControlTable[0]);
    uDMAEnable();
		

  return 0;

  
}

