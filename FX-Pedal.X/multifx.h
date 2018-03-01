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

// Pin definitions
#define SRAM_CS  LATAbits.LATA4     // CS SPI SRAM chip select

#define LCD_D4   LATBbits.LATB8     // LCD data pin 4
#define LCD_D5   LATBbits.LATB9     // LCD data pin 5
#define LCD_D6   LATBbits.LATB10    // LCD data pin 6
#define LCD_D7   LATBbits.LATB11    // LCD data pin 7
#define LCD_EN   LATBbits.LATB12    // LCD EN pin
#define LCD_RS   LATBbits.LATB13    // LCD RS pin

#define LED      LATBbits.LATB4     // Rate LED

#define ROTARY_L PORTAbits.RA2      // Rotary encoder pin L
#define ROTARY_R PORTAbits.RA3      // Rotary encoder pin R

// SRAM chip commands
#define READ  0x03  // Read data from memory array beginning at selected address
#define WRITE 0x02  // Write data to memory array beginning at selected address
#define RDMR  0x05  // Read mode register
#define WRMR  0x01  // Wrrite mode register

// SRAM Modes of operation
#define byte 0
#define seq  64
#define page 128

// Simplified LCD commands
#define LCD_clear() LCD_cmd(0x01)           // Clear display
#define LCD_line1() LCD_setPosition(0x00)   // Change address to write to 1st line
#define LCD_line2() LCD_setPosition(0x40)   // Change address to write to 2nd line

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
void initSPI(void);
void initSRAM(unsigned char sram_mode);
void initLCD(void);
// SRAM
void sram_fill(unsigned int sramdata);
void sram_write(unsigned long sramaddress, unsigned int sramdata);
unsigned int sram_read(unsigned long sramaddress);
// LCD
void LCD_setPosition(unsigned int c);
void LCD_cmd(unsigned int c);
void LCD_pulse(void);
void LCD_upperNibble(unsigned int c);
void LCD_lowerNibble(unsigned int c);
void LCD_putChar(unsigned int c);
void LCD_putString(char *buffer);
void updateLCD(void);
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