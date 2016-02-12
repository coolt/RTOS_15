
#include "set_pinout.h"
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
// from here new for LAB 6
#include "driverlib/epi.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"


//*****************************************************************************
//
// A global variable indicating which daughter board, if any, is currently
// connected to the development board.
//
//*****************************************************************************
tDaughterBoard g_eDaughterType;
//*****************************************************************************
//
// The maximum number of GPIO ports.
//
//*****************************************************************************
#define NUM_GPIO_PORTS  9

//*****************************************************************************
//
// Base addresses of the GPIO ports that may contain EPI signals.  The index
// into this array must correlate with the index in the ucPortIndex field of
// tEPIPinInfo.
//
//*****************************************************************************
const unsigned long g_pulGPIOBase[NUM_GPIO_PORTS] =
{
    GPIO_PORTA_BASE,
    GPIO_PORTB_BASE,
    GPIO_PORTC_BASE,
    GPIO_PORTD_BASE,
    GPIO_PORTE_BASE,
    GPIO_PORTF_BASE,
    GPIO_PORTG_BASE,
    GPIO_PORTH_BASE,
    GPIO_PORTJ_BASE
};

//*****************************************************************************
//
// Structure used to map an EPI signal to a GPIO port and pin on the target
// part.  The ucPortIndex member is the index into the g_pulGPIOBase array
// containing the base address of the port.
//
//*****************************************************************************
typedef struct
{
    unsigned char ucPortIndex;
    unsigned char ucPin;
    unsigned long ulConfig;
}
tEPIPinInfo;

//*****************************************************************************
//
// The maximum number of EPI interface signals (EPI0Sxx).
//
//*****************************************************************************
#define NUM_EPI_SIGNALS 32

//*****************************************************************************
//
// The number of EPI clock periods for a write access with no wait states.
//
//*****************************************************************************
#define EPI_WRITE_CYCLES 4

//*****************************************************************************
//
// The number of EPI clock periods for a read access with no wait states.
//
//*****************************************************************************
#define EPI_READ_CYCLES  4

//*****************************************************************************
//
// The number of EPI clock periods added for each wait state.
//
//*****************************************************************************
#define EPI_WS_CYCLES    2

//*****************************************************************************
//
// This array holds the information necessary to map an EPI signal to a
// particular GPIO port and pin on the target part and also the port control
// nibble required to enable that EPI signal.  The index into the array is the
// EPI signal number.
//
//*****************************************************************************
static const tEPIPinInfo g_psEPIPinInfo[NUM_EPI_SIGNALS] =
{
    { 7, 3, GPIO_PH3_EPI0S0 },
    { 7, 2, GPIO_PH2_EPI0S1 },
    { 2, 4, GPIO_PC4_EPI0S2 },
    { 2, 5, GPIO_PC5_EPI0S3 },
    { 2, 6, GPIO_PC6_EPI0S4 },
    { 2, 7, GPIO_PC7_EPI0S5 },
    { 7, 0, GPIO_PH0_EPI0S6 },
    { 7, 1, GPIO_PH1_EPI0S7 },
    { 4, 0, GPIO_PE0_EPI0S8 },
    { 4, 1, GPIO_PE1_EPI0S9 },
    { 7, 4, GPIO_PH4_EPI0S10 },
    { 7, 5, GPIO_PH5_EPI0S11 },
    { 5, 4, GPIO_PF4_EPI0S12 },
    { 6, 0, GPIO_PG0_EPI0S13 },
    { 6, 1, GPIO_PG1_EPI0S14 },
    { 5, 5, GPIO_PF5_EPI0S15 },
    { 8, 0, GPIO_PJ0_EPI0S16 },
    { 8, 1, GPIO_PJ1_EPI0S17 },
    { 8, 2, GPIO_PJ2_EPI0S18 },
    { 8, 3, GPIO_PJ3_EPI0S19 },
    { 3, 2, GPIO_PD2_EPI0S20 },
    { 3, 3, GPIO_PD3_EPI0S21 },
    { 1, 5, GPIO_PB5_EPI0S22 },
    { 1, 4, GPIO_PB4_EPI0S23 },
    { 4, 2, GPIO_PE2_EPI0S24 },
    { 4, 3, GPIO_PE3_EPI0S25 },
    { 7, 6, GPIO_PH6_EPI0S26 },
    { 7, 7, GPIO_PH7_EPI0S27 },
    { 8, 4, GPIO_PJ4_EPI0S28 },
    { 8, 5, GPIO_PJ5_EPI0S29 },
    { 8, 6, GPIO_PJ6_EPI0S30 },
    { 6, 7, GPIO_PG7_EPI0S31 }
};

//*****************************************************************************
//
// Bit mask defining the EPI signals (EPI0Snn, for 0 <= n < 32) required for
// the default configuration (in this case, we assume the SDRAM daughter board
// is present).
//
//*****************************************************************************
#define EPI_PINS_SDRAM 0xF00FFFFF

//*****************************************************************************
//
// I2C connections for the EEPROM device used on DK daughter boards to provide
// an ID to applications.
//
//*****************************************************************************
#define ID_I2C_PERIPH              (SYSCTL_PERIPH_I2C0)
#define ID_I2C_MASTER_BASE         (I2C0_MASTER_BASE)
#define ID_I2CSCL_GPIO_PERIPH      (SYSCTL_PERIPH_GPIOB)
#define ID_I2CSCL_GPIO_PORT        (GPIO_PORTB_BASE)
#define ID_I2CSCL_PIN              (GPIO_PIN_2)

#define ID_I2CSDA_GPIO_PERIPH      (SYSCTL_PERIPH_GPIOB)
#define ID_I2CSDA_GPIO_PORT        (GPIO_PORTB_BASE)
#define ID_I2CSDA_PIN              (GPIO_PIN_3)

#define ID_I2C_ADDR                0x50

tDaughterIDInfo *psInfo;

    //*****************************************************************************
//
// Determines which daughter board is currently attached to the development
// board and returns the daughter board's information block as
//
// This function determines which of the possible daughter boards are attached
// to the development board.  It recognizes Flash/SRAM and FPGA daughter
// boards, each of which contains an I2C device which may be queried to
// identify the board.  In cases where the SDRAM daughter board is attached,
// this function will return \b NONE and the determination of whether or not
// the board is present is left to function SDRAMInit() in extram.c.
//
// \return Returns \b DAUGHTER_FPGA if the FPGA daugher is detected, \b
// DAUGHTER_SRAM_FLASH if the SRAM and flash board is detected, \b
// DAUGHTER_EM2 if the EM2 LPRF board is detected and \b DAUGHTER_NONE if
//! no board could be identified (covering the cases where either no
//! board or the SDRAM board is attached).
//
//*****************************************************************************
static tDaughterBoard
DaughterBoardTypeGet(tDaughterIDInfo *psInfo)
{
	
    // We experienced an error reading the ID EEPROM or read no valid info
    // structure from the device.  This likely indicates that no daughter
    // board is present.  Set the return structure to configure the system
    // assuming that the default (SDRAM) daughter board is present.
    //
    psInfo->usBoardID = (unsigned short)DAUGHTER_NONE;
    psInfo->ulEPIPins = EPI_PINS_SDRAM;
    psInfo->ucEPIMode = EPI_MODE_SDRAM;
    psInfo->ulConfigFlags = (EPI_SDRAM_FULL_POWER | EPI_SDRAM_SIZE_64MBIT);
    psInfo->ucAddrMap = (EPI_ADDR_RAM_SIZE_256MB | EPI_ADDR_RAM_BASE_6);
    psInfo->usRate0nS = 20;
    psInfo->usRate1nS = 20;
    psInfo->ucRefreshInterval = 64;
    psInfo->usNumRows = 4096;
    return(DAUGHTER_NONE);
}

//*****************************************************************************
//
// Given the system clock rate and a desired EPI rate, calculate the divider
// necessary to set the EPI clock at or lower than but as close as possible to
// the desired rate.  The divider is returned and the desired rate is updated
// to give the actual EPI clock rate (in nanoseconds) that will result from
// the use of the calculated divider.
//
//*****************************************************************************

static unsigned short
EPIDividerFromRate(unsigned short *pusDesiredRate, unsigned long ulClknS)
{
    unsigned long ulDivider, ulDesired;

    //
    // If asked for an EPI clock that is at or above the system clock rate,
    // set the divider to 0 and update the EPI rate to match the system clock
    // rate.
    //
    if((unsigned long)*pusDesiredRate <= ulClknS)
    {
        *pusDesiredRate = (unsigned short)ulClknS;
        return(0);
    }

    //
    // The desired EPI rate is slower than the system clock so determine
    // the divider value to use to achieve this as best we can.  The divider
    // generates the EPI clock using the following formula:
    //
    //                     System Clock
    // EPI Clock =   -----------------------
    //                ((Divider/2) + 1) * 2
    //
    // The formula for ulDivider below is determined by reforming this
    // equation and including a (ulClknS - 1) term to ensure that we round
    // the correct way, generating an EPI clock that is never faster than
    // the requested rate.
    //
    ulDesired = (unsigned long)*pusDesiredRate;
    ulDivider = 2 * ((((ulDesired + (ulClknS - 1)) / ulClknS) / 2) - 1) + 1;

    //
    // Now calculate the actual EPI clock period based on the divider we
    // just chose.
    //
    *pusDesiredRate = (unsigned short)(ulClknS * (2 * ((ulDivider / 2) + 1)));

    //
    // Return the divider we calculated.
    //
    return((unsigned short)ulDivider);
}

//*****************************************************************************
//
// Calculate the divider parameter required by EPIDividerSet() based on the
// current system clock rate and the desired EPI rates supplied in the
// usRate0nS and usRate1nS fields of the daughter board information structure.
//
// The dividers are calculated to ensure that the EPI rate is no faster than
// the requested rate and the rate fields in psInfo are updated to reflect the
// actual rate that will be used based on the calculated divider.
//
//*****************************************************************************
static unsigned long
CalcEPIDivider(tDaughterIDInfo *psInfo, unsigned long ulClknS)
{
    unsigned short usDivider0, usDivider1;
    unsigned short pusRate[2];

    //
    // Calculate the dividers required for the two rates specified.
    //
    pusRate[0] = psInfo->usRate0nS;
    pusRate[1] = psInfo->usRate1nS;
    usDivider0 = EPIDividerFromRate(&(pusRate[0]), ulClknS);
    usDivider1 = EPIDividerFromRate(&(pusRate[1]), ulClknS);
    psInfo->usRate0nS = pusRate[0];
    psInfo->usRate1nS = pusRate[1];

    //
    // Munge the two dividers together into a format suitable to pass to
    // EPIDividerSet().
    //
    return((unsigned long)usDivider0 | (((unsigned long)usDivider1) << 16));
}

//*****************************************************************************
//
// Returns the configuration parameter for EPIConfigHB8Set() based on the
// config flags and read and write access times found in the psInfo structure,
// and the current EPI clock clock rate as found in the usRate0nS field of the
// psInfo structure.
//
// The EPI clock rate is used to determine the number of wait states required
// so CalcEPIDivider() must have been called before this function to ensure
// that the usRate0nS field has been updated to reflect the actual EPI clock in
// use.  Note, also, that there is only a single read and write wait state
// setting even if dual chip selects are in use.  In this case, the caller
// must ensure that the dividers and access times provided generate suitable
// cycles for the devices attached to both chip selects.
//
//*****************************************************************************
static unsigned long
HB8ConfigGet(tDaughterIDInfo *psInfo)
{
    unsigned long ulConfig, ulWrWait, ulRdWait;

    //
    // Start with the config flags provided in the information structure.
    //
    ulConfig = psInfo->ulConfigFlags;

    //
    // How many write wait states do we need?
    //
    if((unsigned long)psInfo->ucWriteAccTime >
       (EPI_WRITE_CYCLES * (unsigned long)psInfo->usRate0nS))
    {
        //
        // The access time is more than 4 EPI clock cycles so we need to
        // introduce some wait states.  How many?
        //
        ulWrWait = (unsigned long)psInfo->ucWriteAccTime -
                   (EPI_WRITE_CYCLES * psInfo->usRate0nS);
        ulWrWait += ((EPI_WS_CYCLES * psInfo->usRate0nS) - 1);
        ulWrWait /= (EPI_WS_CYCLES * psInfo->usRate0nS);

        //
        // The hardware only allows us to specify 0, 1, 2 or 3 wait states.  If
        // we end up with a number greater than 3, we have a problem.  This
        // indicates an error in the daughter board information structure.
        //
        ASSERT(ulWrWait < 4);

        //
        // Set the configuration flag indicating the desired number of write
        // wait states.
        //
        switch(ulWrWait)
        {
            case 0:
            {
                break;
            }

            case 1:
            {
                ulConfig |= EPI_HB8_WRWAIT_1;
                break;
            }

            case 2:
            {
                ulConfig |= EPI_HB8_WRWAIT_2;
                break;
            }

            case 3:
            default:
            {
                ulConfig |= EPI_HB8_WRWAIT_3;
                break;
            }
        }
    }

    //
    // How many read wait states do we need?
    //
    if((unsigned long)psInfo->ucReadAccTime >
       (EPI_READ_CYCLES * (unsigned long)psInfo->usRate0nS))
    {
        //
        // The access time is more than 3 EPI clock cycles so we need to
        // introduce some wait states.  How many?
        //
        ulRdWait = (unsigned long)psInfo->ucReadAccTime -
                   (EPI_READ_CYCLES * psInfo->usRate0nS);
        ulRdWait += ((EPI_WS_CYCLES * psInfo->usRate0nS) - 1);
        ulRdWait /= (EPI_WS_CYCLES * psInfo->usRate0nS);

        //
        // The hardware only allows us to specify 0, 1, 2 or 3 wait states.  If
        // we end up with a number greater than 3, we have a problem.  This
        // indicates an error in the daughter board information structure.
        //
        ASSERT(ulRdWait < 4);

        //
        // Set the configuration flag indicating the desired number of read
        // wait states.
        //
        switch(ulRdWait)
        {
            case 0:
            {
                break;
            }

            case 1:
            {
                ulConfig |= EPI_HB8_RDWAIT_1;
                break;
            }

            case 2:
            {
                ulConfig |= EPI_HB8_RDWAIT_2;
                break;
            }

            case 3:
            default:
            {
                ulConfig |= EPI_HB8_RDWAIT_3;
                break;
            }
        }
    }

    //
    // Return the configuration flags back to the caller.
    //
    return(ulConfig);
}

//*****************************************************************************
//
// Returns the configuration parameters for EPIConfigSDRAMSet() based on the
// config flags, device size and refresh interval provided in psInfo and the
// system clock rate provided in ulClkHz.
//
//*****************************************************************************
static unsigned long
SDRAMConfigGet(tDaughterIDInfo *psInfo, unsigned long ulClkHz,
               unsigned long *pulRefresh)
{
    unsigned long ulConfig;

    //
    // Start with the config flags provided to us.
    //
    ulConfig = psInfo->ulConfigFlags;

    //
    // Set the SDRAM core frequency depending upon the system clock rate.
    //
    if(ulClkHz < 15000000)
    {
        ulConfig |= EPI_SDRAM_CORE_FREQ_0_15;
    }
    else if(ulClkHz < 30000000)
    {
        ulConfig |= EPI_SDRAM_CORE_FREQ_15_30;
    }
    else if(ulClkHz < 50000000)
    {
        ulConfig |= EPI_SDRAM_CORE_FREQ_30_50;
    }
    else
    {
        ulConfig |= EPI_SDRAM_CORE_FREQ_50_100;
    }

    //
    // Now determine the correct refresh count required to refresh the entire
    // device in the time specified.
    //
    *pulRefresh = ((ulClkHz / psInfo->usNumRows) *
                  (unsigned long)psInfo->ucRefreshInterval) / 1000;

    //
    // Return the calculated configuration parameter to the caller.
    //
    return(ulConfig);
}

//*****************************************************************************
//
// Configures all pins associated with the Extended Peripheral Interface (EPI).
//
// \param eDaughter identifies the attached daughter board (if any).
//
// This function configures all pins forming part of the EPI on the device and
// configures the EPI peripheral appropriately for whichever hardware we
// detect is connected to it. On exit, the EPI peripheral is enabled and all
// pins associated with the interface are configured as EPI signals. Drive
// strength is set to 8mA.
//
//*****************************************************************************
static void
EPIPinConfigSet(tDaughterIDInfo *psInfo)
{
    unsigned long ulLoop, ulClk, ulNsPerTick, ulRefresh;
    unsigned char pucPins[NUM_GPIO_PORTS];

    //
    // Enable the EPI peripheral
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_EPI0);

    //
    // Clear our pin bit mask array.
    //
    for(ulLoop = 0; ulLoop < NUM_GPIO_PORTS; ulLoop++)
    {
        pucPins[ulLoop] = 0;
    }

    //
    // Determine the pin bit masks for the EPI pins for each GPIO port.
    //
    for(ulLoop = 0; ulLoop < NUM_EPI_SIGNALS; ulLoop++)
    {
        //
        // Is this EPI signal required?
        //
        if(psInfo->ulEPIPins & (1 << ulLoop))
        {
            //
            // Yes - set the appropriate bit in our pin bit mask array.
            //
            pucPins[g_psEPIPinInfo[ulLoop].ucPortIndex] |=
                (1 << g_psEPIPinInfo[ulLoop].ucPin);
        }
    }

    //
    // At this point, pucPins contains bit masks for each GPIO port with 1s in
    // the positions of every required EPI signal.  Now we need to configure
    // those pins appropriately.  Cycle through each port configuring EPI pins
    // in any port which contains them.
    //
    for(ulLoop = 0; ulLoop < NUM_GPIO_PORTS; ulLoop++)
    {
        //
        // Are there any EPI pins used in this port?
        //
        if(pucPins[ulLoop])
        {
            //
            // Yes - configure the EPI pins.
            //
            ROM_GPIOPadConfigSet(g_pulGPIOBase[ulLoop], pucPins[ulLoop],
                                 GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
            ROM_GPIODirModeSet(g_pulGPIOBase[ulLoop], pucPins[ulLoop],
                               GPIO_DIR_MODE_HW);
        }
    }

    //
    // Now set the EPI operating mode for the daughter board detected.  We need
    // to determine some timing information based on the ID block we have and
    // also the current system clock.
    //
    ulClk = ROM_SysCtlClockGet();
    ulNsPerTick = 1000000000 / ulClk;

    //
    // If the EPI is not disabled (the daughter board may, for example, want
    // to use all the pins for GPIO), configure the interface as required.
    //
    if(psInfo->ucEPIMode != EPI_MODE_DISABLE)
    {
        //
        // Set the EPI clock divider to ensure a basic EPI clock rate no faster
        // than defined via the ucRate0nS and ucRate1nS fields in the info
        // structure.
        //
        EPIDividerSet(EPI0_BASE, CalcEPIDivider(psInfo, ulNsPerTick));

        //
        // Set the basic EPI operating mode based on the value from the info
        // structure.
        //
        EPIModeSet(EPI0_BASE, psInfo->ucEPIMode);

        //
        // Carry out mode-dependent configuration.
        //
        switch(psInfo->ucEPIMode)
        {
            //
            // The daughter board must be configured for SDRAM operation.
            //
            case EPI_MODE_SDRAM:
            {
                //
                // Work out the SDRAM configuration settings based on the
                // supplied ID structure and system clock rate.
                //
                ulLoop = SDRAMConfigGet(psInfo, ulClk, &ulRefresh);

                //
                // Set the SDRAM configuration.
                //
                EPIConfigSDRAMSet(EPI0_BASE, ulLoop, ulRefresh);
                break;
            }

            //
            // The daughter board must be configured for HostBus8 operation.
            //
            case EPI_MODE_HB8:
            {
                //
                // Determine the number of read and write wait states required
                // to meet the supplied access timing.
                //
                ulLoop = HB8ConfigGet(psInfo);

                //
                // Set the HostBus8 configuration.
                //
                EPIConfigHB8Set(EPI0_BASE, ulLoop, psInfo->ucMaxWait);
                break;
            }

            //
            // The daughter board must be configured for Non-Moded/General
            // Purpose operation.
            //
            case EPI_MODE_GENERAL:
            {
                EPIConfigGPModeSet(EPI0_BASE, psInfo->ulConfigFlags,
                                   psInfo->ucFrameCount, psInfo->ucMaxWait);
                break;
            }
        }

        //
        // Set the EPI address mapping.
        //
        EPIAddressMapSet(EPI0_BASE, psInfo->ucAddrMap);
    }
}

//*****************************************************************************
//
// Set the GPIO port control registers appropriately for the hardware.
//
// This function determines the correct port control settings to enable the
// basic peripheral signals for the development board on their respective pins
// and also ensures that all required EPI signals are correctly routed.  The
// EPI signal configuration is determined from the daughter board information
// structure passed via the \e psInfo parameter.
//
//*****************************************************************************
static void
PortControlSet(tDaughterIDInfo *psInfo)
{
    unsigned long ulLoop;

    //
    // To begin with, we set the port control values for all the non-EPI
    // peripherals.
    //

    //
    // GPIO Port A pins
    //
    // To use CAN0, these calls must be changed.  This enables USB
    // functionality instead of CAN. For CAN, use:
    //
    //     GPIOPinConfigure(GPIO_PA6_CAN0RX);
    //     GPIOPinConfigure(GPIO_PA7_CAN0TX);
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);
    GPIOPinConfigure(GPIO_PA6_USB0EPEN);
    GPIOPinConfigure(GPIO_PA7_USB0PFLT);

    //
    // GPIO Port B pins
    //
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    GPIOPinConfigure(GPIO_PB6_I2S0TXSCK);
    GPIOPinConfigure(GPIO_PB7_NMI);

    //
    // GPIO Port D pins.
    //
    GPIOPinConfigure(GPIO_PD0_I2S0RXSCK);
    GPIOPinConfigure(GPIO_PD1_I2S0RXWS);
    GPIOPinConfigure(GPIO_PD4_I2S0RXSD);
    GPIOPinConfigure(GPIO_PD5_I2S0RXMCLK);

    //
    // GPIO Port E pins
    //
    GPIOPinConfigure(GPIO_PE4_I2S0TXWS);
    GPIOPinConfigure(GPIO_PE5_I2S0TXSD);

    //
    // GPIO Port F pins
    //
    GPIOPinConfigure(GPIO_PF1_I2S0TXMCLK);
    GPIOPinConfigure(GPIO_PF2_LED1);
    GPIOPinConfigure(GPIO_PF3_LED0);

    //
    // Now we configure each of the EPI pins if it is needed.
    //
    for(ulLoop = 0; ulLoop < NUM_EPI_SIGNALS; ulLoop++)
    {
        //
        // Is this EPI pin used by this daughter board?
        //
        if(psInfo->ulEPIPins & (1 << ulLoop))
        {
            //
            // Yes - configure the corresponding pin.
            //
            GPIOPinConfigure(g_psEPIPinInfo[ulLoop].ulConfig);
        }
    }
}

//*****************************************************************************
//
//! Configures the device pinout for the development board.
//!
//! This function configures each pin of the device to route the! appropriate
//! peripheral signal as required by the design of the development board.
//!
//! \note This module can be built in two ways.  If the label SIMPLE_PINOUT_SET
//! is not defined, the PinoutSet() function will attempt to read an I2C EEPROM
//! to determine which daughter board is attached to the development kit board
//! and use information from that EEPROM to dynamically configure the EPI
//! appropriately.  In this case, if no EEPROM is found, the EPI configuration
//! will default to that required to use the SDRAM daughter board which is
//! included with the base development kit.
//!
//! If SIMPLE_PINOUT_SET is defined, however, all the dynamic configuration
//! code is replaced with a very simple function which merely sets the pinout
//! and EPI configuration statically.  This is a better representation of how a
//! real-world application would likely initialize the pinout and EPI timing
//! and takes significantly less code space than the dynamic, daughter-board
//! detecting version.  The example offered here sets the pinout and EPI
//! configuration appropriately for the Flash/SRAM/LCD daughter board.
//!
//! \return None.
//
//*****************************************************************************
void
LCDPinoutSet(void)
{
    tDaughterIDInfo sInfo;

    //
    // Enable all GPIO banks.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    //
    // Determine which daughter board (if any) is currently attached to the
    // development board.
    //
    g_eDaughterType = DaughterBoardTypeGet(&sInfo);

    //
    // Determine the port control settings required to enable the EPI pins
    // and other peripheral signals for this daughter board and set all the
    // GPIO port control registers.
    //
    PortControlSet(&sInfo);

    //
    // Set the pin configuration for the Extended Peripheral Interface (EPI)
    //
    EPIPinConfigSet(&sInfo);
}




