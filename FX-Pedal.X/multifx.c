/* Configuration Bit Settings */
// FBS
#pragma config BWRP = WRPROTECT_OFF     // Boot Segment Write Protect (Boot Segment may be written)
#pragma config BSS = NO_FLASH           // Boot Segment Program Flash Code Protection (No Boot program Flash segment)
#pragma config RBS = NO_RAM             // Boot Segment RAM Protection (No Boot RAM)
// FSS
#pragma config SWRP = WRPROTECT_OFF     // Secure Segment Program Write Protect (Secure segment may be written)
#pragma config SSS = NO_FLASH           // Secure Segment Program Flash Code Protection (No Secure Segment)
#pragma config RSS = NO_RAM             // Secure Segment Data RAM Protection (No Secure RAM)
// FGS
#pragma config GWRP = OFF               // General Code Segment Write Protect (User program memory is not write-protected)
#pragma config GSS = OFF                // General Segment Code Protection (User program memory is not code-protected)
// FOSCSEL
#pragma config FNOSC = FRC              // Oscillator Mode (Internal Fast RC (FRC))
#pragma config IESO = ON                // Internal External Switch Over Mode (Start-up device with FRC, then automatically switch to user-selected oscillator source when ready)
// FOSC
#pragma config POSCMD = NONE            // Primary Oscillator Source (Primary Oscillator Disabled)
#pragma config OSCIOFNC = ON            // OSC2 Pin Function (OSC2 pin has digital I/O function)
#pragma config IOL1WAY = OFF             
#pragma config FCKSM = CSECMD           // Clock Switching and Monitor (Clock switching is enabled, Fail-Safe Clock Monitor is disabled)
// FWDT
#pragma config WDTPOST = PS32768        // Watchdog Timer Postscaler (1:32,768)
#pragma config WDTPRE = PR128           // WDT Prescaler (1:128)
#pragma config WINDIS = OFF             // Watchdog Timer Window (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (Watchdog timer enabled/disabled by user software)
// FPOR
#pragma config FPWRT = PWR128           // POR Timer Value (128ms)
#pragma config ALTI2C = OFF             // Alternate I2C  pins (I2C mapped to SDA1/SCL1 pins)
// FICD
#pragma config ICS = PGD1               // Comm Channel Select (Communicate on PGC1/EMUC1 and PGD1/EMUD1)
#pragma config JTAGEN = OFF             // JTAG Port Enable (JTAG is Disabled)

/* Libraries */
#include "p33fxxxx.h"
#include "multifx.h"
#include "SRAM.h"
#include "LCD.h"
#include "Delay.h"
#include "Distortion.h"
#include "Tremolo.h"
#include "Chorus.h"
#include <xc.h>
#include <libpic30.h>
#include <stdio.h>
#include <stdbool.h>
#include <dsp.h>


mode_t fx_mode;

void set_fx_mode(mode_t mode) { fx_mode = mode; }   // set current fx mode
mode_t get_fx_mode(void) { return fx_mode; }        // get current fx mode
void rate_led_enabled(bool state) { LED = state; }  // set rate LED on or OFF

void initOsc(void)
{
    PLLFBD=41;              // PLL prescaler:  M = 43
    CLKDIVbits.PLLPOST = 0; // PLL postscaler: N2 = 2
    CLKDIVbits.PLLPRE = 0;  // PLL divisor:    N1 = 2
    OSCTUN=0;				// Tune FRC oscillator
    	
	RCONbits.SWDTEN=0;      // Disable Watch Dog Timer

	// Clock switch to incorporate PLL
	__builtin_write_OSCCONH(0x01);				// Initiate Clock Switch to
												// FRC with PLL (NOSC=0b001)
	__builtin_write_OSCCONL(0x01);				// Start clock switching
	while (OSCCONbits.COSC != 0b001);			// Wait for Clock switch to occur
	while(OSCCONbits.LOCK!=1) {};               // Wait for PLL to lock
}

void initIO(void)
{
    ADPCFG = 0xFFFB; // all PORTB = Digital except RB2 = analogue
    
    TRISBbits.TRISB4 = 0;   // RB4 as output for Rate LED
    rate_led_enabled(true);   // LED initially off 
    
    // Encoder inputs
    TRISAbits.TRISA2 = 1;
    TRISAbits.TRISA3 = 1;
}

void setDefaults(void)
{
    // TODO: All of these functions accept a percentage as the argument - they should be renamed to make this obvious
    distortion_set_percentage(90);
    distortion_set_symetric(true);
    tremolo_set_period(50);
    chorus_set_period(50);
    delay_set_delay_time(50);
    
    // TODO: this function sets to a discrete level - 0-16... could this be more intuitive?
    delay_set_decay(8);
}