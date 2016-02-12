
#include "inc/hw_types.h"	
#include "inc/hw_memmap.h"

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"

void ledInit_hal( void ) {

  SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOF );
  GPIOPinTypeGPIOOutput( GPIO_PORTF_BASE, GPIO_PIN_3 );
	GPIOPinTypeGPIOOutput( GPIO_PORTF_BASE, GPIO_PIN_2 );

  GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3 );   // On

}

void ledOff_hal( void ) {
 GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_3, 0 );
	GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_2, 0 );
}

void ledOn_hal( void ) {
 GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3 );
	GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2 );
}

void led_gn_off_hal( void ) {
 GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2 );
}

void led_gn_on_hal( void ) {
 GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_2, 0 );
}

void led_or_off_hal( void ) {
	GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3 );
}

void led_or_on_hal( void ) {
	GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_3, 0 );
}

