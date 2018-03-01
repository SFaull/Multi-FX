#include "p33fxxxx.h"
#include "multifx.h"
#include <xc.h>
#include <libpic30.h>
#include <stdio.h>
#include <dsp.h>

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
    extern int MODE;                      // Current effect mode number
    extern int DMAflag;                   // DMA Flag
    extern int i;                         // Counter variable
    fractional output_sample;             // Stores modified sample to be written to DAC
    
    /* Main loop */
    while (1)
    {        
        if(DMAflag)    // When DMA has filled the buffer
        {  
            for(i = 0; i < NUMSAMP; i++)    // Cycle through all samples in the buffer
            {
                while(DAC1STATbits.LEMPTY != 1);    // Wait for D/A conversion

                // Main effects 
                if (MODE==1)        // No effect
                    output_sample = Buffer[i];
                else if (MODE==2)   // Distortion
                    output_sample = distortion(Buffer[i]);	
                else if (MODE==3)   // Tremolo
                    output_sample = tremolo(Buffer[i]);
                else if (MODE==4)   // Delay
                    output_sample = delay(Buffer[i]);              
                else if (MODE==5)   // Chorus
                    output_sample = chorus(Buffer[i]);
                else                // If unknown state is reached revert to mode 1
                    MODE = 1;

                DAC1LDAT = output_sample;      // Load the DAC buffer with data
                DMAflag = 0;                   // Reset DMA flag
            }	
        }	
	} // end while(1)
    
	return 0;

} // end main