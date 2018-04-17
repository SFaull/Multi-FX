#include "p33fxxxx.h"

#include <xc.h>
#include <libpic30.h>
#include <stdio.h>
#include <dsp.h>

#include "multifx.h"
#include "SRAM.h"
#include "LCD.h"
#include "Serial.h"
#include "Audio.h"
#include "Delay.h"
#include "Distortion.h"
#include "Tremolo.h"
#include "Chorus.h"

int main (void)
{
    /* Configuration function initializations */
    initOsc();      // Initialize the system clock
    initIO();       // Initialize any input/output pins not covered by other functions
    initUART();     // Initialize UART communication
   	initADC();      // Initialize the A/D converter
	initDAC(); 		// Initialize the D/A converter 
	initDMA();		// Initialize the DMA controller to buffer ADC data in conversion order
	initTimer3();	// Initialize Timer3 to generate sampling event for ADC
    initTimer1();   // Initialise Timer1 to check rotary encoder pin periodically 
    initSPI();      // Initialise SPI for use with SRAM
    initSRAM(seq);  // Setup SRAM in Sequential mode
    initLCD();      // Initialise the LCD
    updateLCD();    // Update the display for startup
    
    /* Variable initializations */
    extern fractional Buffer[NUMSAMP];	  // Sample buffer
    extern int DMAflag;                   // DMA Flag
    fractional output_sample;             // Stores modified sample to be written to DAC
    
    /* Main loop */
    while (1)
    {        
        if(DMAflag)    // When DMA has filled the buffer
        {  
            int i;
            for(i = 0; i < NUMSAMP; i++)    // Cycle through all samples in the buffer
            {
                while(DAC1STATbits.LEMPTY != 1);    // Wait for D/A conversion

                mode_t mode = getMode();
                switch(mode)
                {
                    case kMode_clean:
                        output_sample = Buffer[i];
                        break;
                    case kMode_distortion:
                        output_sample = distortion(Buffer[i]);	
                        break;
                    case kMode_tremolo:
                        output_sample = tremolo(Buffer[i]);
                        break;
                    case kMode_delay:
                        output_sample = delay(Buffer[i]);  
                        break;
                    case kMode_chorus:
                        output_sample = chorus(Buffer[i]);
                        break;
                    default:
                        setMode(kMode_clean);
                        break;
                }

                DAC1LDAT = output_sample;      // Load the DAC buffer with data
                DMAflag = 0;                   // Reset DMA flag
            }	
        }	
	} // end while(1)
    
	return 0;

} // end main