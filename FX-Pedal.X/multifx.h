/* Definitions */
#define FOSC		79227500                // Clock-frequecy in Hz -> Fin*(M/(N1*N2)) = 7.37*(43/(2*2)) = 79227500 
#define FCY      	(FOSC/2)                // MCU is running at FCY MIPS (must be defined correctly to use "__delay_ms()" func)
#define BAUDRATE    9600                    // Baud rate for UART
#define BRGVAL      ((FCY/BAUDRATE)/16)-1   // BRG value calculated from baud rate definition
#define Fs   		44211                   // Sampling frequency
#define SAMPPRD     (FCY/Fs)-1              // Sampling period
#define NUMSAMP     128                     // Number of samples in sample buffer - MUST NOT EXCEED 512
#define ENCFREQ     500                     // Frequency to check encoder pin
#define ENCPRD      ((FCY/256)/ENCFREQ)-1   // Period to check encoder pin

#define LED      LATBbits.LATB4     // Rate LED

#define ROTARY_L PORTAbits.RA2      // Rotary encoder pin L
#define ROTARY_R PORTAbits.RA3      // Rotary encoder pin R

/* Function declarations */
// Initialisations
void initOsc(void);
void initIO(void);
void initUART(void);
void initADC(void);
void initDAC(void);
void initDMA(void);
void initTimer3(void);
void initTimer1(void);




// Encoder
void knobTurned(int L, int R);

// General
void applyCom(void);
signed int volume(signed int x, int level);
signed int distortion(signed int dist_in);
signed int delay(signed int delay_in);
signed int chorus(signed int chorus_in);
signed int tremolo(signed int trem_in);
signed int lowpass(signed int lp_in);

// Interrupts
void __attribute__((interrupt, no_auto_psv)) _DMA0Interrupt(void);
void __attribute__((interrupt, no_auto_psv)) _DAC1RInterrupt(void);
void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void);
void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void);
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void);