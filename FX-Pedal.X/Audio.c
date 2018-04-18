#include "Audio.h"
#include "p33fxxxx.h"
#include "multifx.h"
#include <xc.h>
#include <libpic30.h>
#include <stdio.h>
#include <dsp.h>

// Sample buffer populated by DMA
fractional Buffer[NUMSAMP] __attribute__((space(dma)));
int DMAflag = 0;    // DMA flag

void initADC(void)
{
	AD1CON1bits.FORM = 3; 			// Data Output Format: Signed Fraction (Q15 format)
	AD1CON1bits.SSRC = 2;			// Sample Clock Source: GP Timer starts conversion
	AD1CON1bits.ASAM = 1;			// ADC Sample Control: Sampling begins immediately after conversion
	AD1CON1bits.AD12B = 1;			// 12-bit ADC operation
	AD1CON2bits.CHPS = 0;			// Converts CH0
	AD1CON3bits.ADRC = 0;			// ADC Clock is derived from Systems Clock
	AD1CON3bits.ADCS = 3;			// ADC Conversion Clock Tad=Tcy*(ADCS+1)= (1/40M)*4 = 100ns 
									// ADC Conversion Time for 12-bit Tc=14*Tad = 1.4us 		
	AD1CON1bits.ADDMABM = 1; 		// DMA buffers are built in conversion order mode
	AD1CON2bits.SMPI = 0;			// SMPI must be 0
    
    //AD1CHS0: A/D Input Select Register
    AD1CHS0bits.CH0SA = 4;			// MUXA +ve input selection (AN4) for CH0
	AD1CHS0bits.CH0NA = 0;			// MUXA -ve input selection (Vref-) for CH0
    
    //AD1PCFGH/AD1PCFGL: Port Configuration Register
	AD1PCFGL=0xFFFF;
    AD1PCFGLbits.PCFG4 = 0;			// AN4 as Analog Input   
    IFS0bits.AD1IF = 0;				// Clear the A/D interrupt flag bit
    IEC0bits.AD1IE = 0;				// Do Not Enable A/D interrupt 
    AD1CON1bits.ADON = 1;			// Turn on the A/D converter  
}

void initDAC(void)
{
	/* Initiate DAC Clock */
	ACLKCONbits.SELACLK = 0;		// FRC w/ Pll as Clock Source 
	ACLKCONbits.AOSCMD = 0;			// Auxiliary Oscillator Disabled
	ACLKCONbits.ASRCSEL = 0;		// Auxiliary Oscillator is the Clock Source
	ACLKCONbits.APSTSCLR = 7;		// FRC divide by 1 
	DAC1STATbits.LOEN = 1;			// LEFT Channel DAC Output Enabled  
	DAC1DFLT = 0x8000;				// DAC Default value is the midpoint 
	DAC1CONbits.DACFDIV = 13; 		//(44.211kHz) - Divide High Speed Clock by DACFDIV+1 
	DAC1CONbits.FORM = 1;			// Data Format is signed integer
	DAC1CONbits.AMPON = 0;			// Analog Output Amplifier is enabled during Sleep Mode/Stop-in Idle mode
	DAC1CONbits.DACEN = 1;			// DAC1 Module Enabled 
}

void initTimer3(void)
{
	TMR3 = 0x0000;					// Clear TMR3
	PR3 = SAMPPRD;					// Load period value in PR3
	IFS0bits.T3IF = 0;				// Clear Timer 3 Interrupt Flag
	IEC0bits.T3IE = 0;				// Clear Timer 3 interrupt enable bit	
	T3CONbits.TON = 1;				// Enable Timer 3
}

void initDMA(void)
{
	DMA0CONbits.AMODE = 0;			// Configure DMA for Register indirect with post increment
    DMA0CONbits.MODE = 0;			// Configure DMA for Continuous Ping-Pong mode disabled
	DMA0PAD = (int)&ADC1BUF0;		// Peripheral Address Register: ADC buffer
	DMA0CNT = (NUMSAMP-1);			// DMA Transfer Count is (NUMSAMP-1)	
	DMA0REQ = 13;					// ADC interrupt selected for DMA channel IRQ
	DMA0STA = __builtin_dmaoffset(Buffer);	// DMA RAM Start Address A	
	IFS0bits.DMA0IF = 0;			// Clear the DMA interrupt flag bit
    IEC0bits.DMA0IE = 1;			// Set the DMA interrupt enable bit
	DMA0CONbits.CHEN = 1;			// Enable DMA channel
}

void __attribute__((interrupt, no_auto_psv)) _DMA0Interrupt(void)
{
	DMAflag = 1;                    // Buffer full flag
 	IFS0bits.DMA0IF = 0;			// Clear the DMA0 Interrupt Flag
}

void __attribute__((interrupt, no_auto_psv)) _DAC1RInterrupt(void)
{
	IFS4bits.DAC1LIF = 0; 			// Clear LEFT Channel Interrupt Flag 	 
}