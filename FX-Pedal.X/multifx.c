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
#include <xc.h>
#include <libpic30.h>
#include <stdio.h>
#include <dsp.h>

// Sample buffer populated by DMA
fractional Buffer[NUMSAMP] __attribute__((space(dma)));
int MODE = 1; // effect mode (ranges from 1 to 5)

// Effect variables
int tremPeriod = 200;   // tremolo period used to determine the frequency (default value)
int chorusPeriod = 195; // chorus period used to determine the frequency (default value)
int delayVol = 8;       // default volume for delay decay (8/16 -> 50%)
// Distortion thresholds
fractional Pthreshold = Q15(0.175);   // +ve Cutoff Converted to Q15
fractional Nthreshold = Q15(-0.175);  // -ve Cutoff Converted to Q15
int symmetric = 0;                    // flag to indicate whether clipping is symmetric or asymmetric

// Parameters for various effects, later converted to strings for LCD
int   dist_th = 50;     // threshold as a percentage
float trem_speed = 5;   // tremolo speed in Hz
int   delay_time = 741; // delay time is seconds
float delay_decay = 50; // delay decay as percentage
float chorus_speed = 3; // chorus speed in Hz

// Generic variables
int i;              // counter variable
int DMAflag = 0;    // DMA flag

// SRAM address pointers
unsigned long write_address = 0;
unsigned long read_address = 65536;   // half of maximum value.. this is simply the default value

// Bluetooth communication storage
unsigned char identifier = '\0';    // character that indicated operation to perform
unsigned char message[2] = "";      // message holds data, usually as percentage
unsigned int  msgCounter = 0;       // counter for received message


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
    LED = 0;                // LED initially off 
    
    // Encoder inputs
    TRISAbits.TRISA2 = 1;
    TRISAbits.TRISA3 = 1;
}

void initUART(void)
{
    // PERIPHERAL PIN SELECT
    __builtin_write_OSCCONL(OSCCON & ~(1<<6));  // Unlock the registers
    RPINR18bits.U1RXR = 1;  // Assign U1Rx To Pin RP1
    RPOR0bits.RP0R = 3; // Assign U1Tx To Pin RP0
    __builtin_write_OSCCONL(OSCCON | (1<<6));   // Lock the registers

    U1BRG = BRGVAL; // Set baud rate to 9600
   
    U1MODEbits.STSEL = 0; // 1 Stop bit
    U1MODEbits.PDSEL = 0; // No Parity, 8 data bits
    U1MODEbits.ABAUD = 0; // Auto-Baud Disabled
    U1MODEbits.BRGH = 0;  // Low Speed mode
    
    U1STAbits.UTXISEL0 = 0; // Interrupt after one TX Character is transmitted
    U1STAbits.UTXISEL1 = 0;
    IFS0bits.U1RXIF = 0;    // clear Rx interrupt flag
    IFS0bits.U1TXIF = 0;    // clear Tx interrupt flag
    IEC0bits.U1TXIE = 1;    // Enable Tx Interrupt
    IEC0bits.U1RXIE = 1;    // Enable Rx interrupt
    
    U1MODEbits.UARTEN =   1;     // Enable UART
    U1STAbits.UTXEN   =   1;     // Enable UART TX
    
    /* wait at least 104 usec (1/9600) before sending first char */
    __delay_ms(120);
}

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


void initTimer1(void)   // Timer to periodically check encoder pin
{
    T1CONbits.TON = 0;      // Disable Timer
    T1CONbits.TCS = 0;      // Select internal instruction cycle clock
    T1CONbits.TGATE = 0;    // Disable Gated Timer mode
    T1CONbits.TCKPS = 0b11; // Select 1:256 Prescaler
    TMR1 = 0x00;            // Clear timer register
    PR1 = ENCPRD;           // Load the period value
    IPC0bits.T1IP = 0x01;   // Set Timer1 Interrupt Priority Level
    IFS0bits.T1IF = 0;      // Clear Timer1 Interrupt Flag
    IEC0bits.T1IE = 1;      // Enable Timer1 interrupt
    T1CONbits.TON = 1;      // Start Timer
}

// Run when Timer1 interrupt is triggered
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{    
    static int enc;         // !encoder pin value
    static int last_enc;    // previous !encoder pin value
    static int wait;        // flag used to debounce
    static int cnt;         // counter used to debounce

    if (wait==1) // wait flag enabled, begin counter (debounce)
    {
        if (cnt == 99)  // 99 counts equates to 0.2 seconds
        {
            cnt = 0;    // reset counter
            wait = 0;   // reset wait flag
        }
        else
            cnt++;      // increment counter
    }
    else    // otherwise, check the encoder pin
    {
        last_enc = enc;     // store last value
        enc = !(ROTARY_L);  // store encoder pin value

        if( (enc == 0) && (last_enc == 1) ) // If a change has occured
        {
          knobTurned(ROTARY_L, ROTARY_R);   // send values to knobturned function
          wait = 1; // set wait flag so no turns are registered for 0.2 seconds (debounce)
        }
    } 
    
    IFS0bits.T1IF = 0; // Clear Timer1 Interrupt Flag
}



void knobTurned(int L, int R)
{    
    static int state = 0;           // will store two bits for pins A & B on the encoder which we will get from the pins above
    static int bump[] = {0,0,1,-1};
    
    state = 0;          // reset each time
    state = state + L;  // add the state of Pin L
    state <<= 1;        // shift the bit over one spot
    state = state + R;  // add the state of Pin R
 
     // posible states:
     //00 & 01 - something is wrong, do nothing
     //10 - knob was turned forwards
     //11 - knob was turned backwards
     
     MODE = MODE + bump[state]; // Add direction from bump position to MODE
     
     if (MODE > 5)      // dont exceed maximum mode value
         MODE = 1;      // loop round instead
     else if (MODE < 1) // dont go lower than the minimum mode value
         MODE = 5;      // loop round instead

     LED = 0;       // turn rate LED off
     updateLCD();   // update LCD because mode has changed

}

// This function attenuates the signal. Division of the sampled value is used to attenuate the sample
// and 16 linear discrete volume levels are defined 
signed int volume(signed int x, int level)
{
  switch (level)
  {              
     case  0: // 0% Volume
             return 0;   
             break;
     case  1: // 6.25% Volume
             return ( (x>>4) );          
             break;
     case  2: // 12.5% Volume
             return ( (x>>3) );          
             break;
     case  3:  // 18.75% Volume
             return ( (x>>4)+ (x>>3) );        
             break;         
     case  4:  // 25% Volume
             return ( (x>>2) );        
             break;
     case  5:  // 31.25% Volume
             return ( (x>>2)+(x>>4) );       
             break;
   	 case  6:  // 37.5% Volume
             return ( (x>>2)+(x>>3) );       
             break;
     case  7:  // 43.75% Volume
             return ( (x>>2)+(x>>4)+(x>>3) );     
             break;
     case  8:  // 50% Volume
             return ( (x>>1) );     
             break;        
     case  9: // 56.25% Volume
             return ( (x>>1)+(x>>4) );
             break;
     case 10: // 62.5% Volume
             return ( (x>>1)+(x>>3) );       
             break;
     case 11: // 68.75% Volume
             return ( (x>>1)+(x>>4)+(x>>3) );         
             break;
     case 12:  // 75% Volume
             return ( (x>>1)+ (x>>2) );        
             break;         
     case 13:  // 81.25% Volume
             return ( (x>>1)+(x>>2)+(x>>4) );          
             break;
     case 14:  // 87.5% Volume
             return ( (x>>1)+(x>>2)+(x>>3) );    
             break;
   	 case 15:  // 93.75% Volume
             return ( (x>>1)+(x>>2)+(x>>4)+(x>>3) );       
             break;
     case 16:  // 100% Volume
            return x; 
            break;
      default:  // should never happen...
          return 0;
          break;
  }  
}

void initSPI(void)
{ 
    // PERIPHERAL PIN SELECT
    __builtin_write_OSCCONL(OSCCON & ~(1<<6));  // Unlock the registers
    RPINR20bits.SDI1R = 5; //RP5 (pin 14) = SDI
    RPOR3bits.RP6R = 7; //RP6 (pin 15) = SDO
    RPOR3bits.RP7R = 8; //RP7 (pin 16) = SCK
    __builtin_write_OSCCONL(OSCCON | (1<<6));   //Lock the registers
    
    TRISAbits.TRISA4 = 0; // RA4 as output for !CS

    SRAM_CS = 1;                //disable sram
    SPI1CON1bits.DISSCK = 0;    //clock enabled (Internal serial clock is enabled)
    SPI1CON1bits.DISSDO = 0;    //SDO enabled (SDOx pin is controlled by the module)
    SPI1CON1bits.MODE16 = 0;    //8 bit mode (set to 1 for 16 bit)
    SPI1CON1bits.SMP = 0;       //data sampled in middle (Input data is sampled at the middle of data output time)
    SPI1CON1bits.CKE = 1;       //data changes on falling edge (active to idle)
    SPI1CON1bits.CKP = 0;       //idle is low
    SPI1CON1bits.PPRE = 2;      //4:1 primary prescaler
    SPI1CON1bits.SPRE = 7;      //1:1 secondary prescaler (10Mhz SPI clock)
    SPI1CON1bits.MSTEN = 1;     //master mode
    SPI1STATbits.SPIEN = 1;     //enable SPI        
}

void sram_fill(unsigned int sramdata) 
{
    unsigned char temp;
    unsigned long a;
    
    SRAM_CS = 0;                    // Enable SRAM

    SPI1BUF = WRITE;                // Send WRITE command, to be followed by 24-bit address
    while (!SPI1STATbits.SPIRBF);   // Wait for receive buffer to indicate that its full
    temp = SPI1BUF;                 // Store whatever has been put in the receive buffer

    for (a=0; a<3; a++)             // send 24 bit address in 3 bytes
    {
        SPI1BUF = 0;                    
        while (!SPI1STATbits.SPIRBF);
        temp = SPI1BUF;
    }
    
    for (a=0; a<131071; a++)            // write byte to entire of memory
    {
        SPI1BUF = sramdata;             // Send byte
        while (!SPI1STATbits.SPIRBF);
        temp = SPI1BUF;
    }

    SRAM_CS = 1;                         // Disable SRAM

}

void initSRAM(unsigned char sram_mode)
{
    unsigned char temp;

    SRAM_CS = 0;                    // Enable SRAM
    SPI1BUF = WRMR;                 // Put into WRITE mode
    while (!SPI1STATbits.SPIRBF);   // Wait for receive buffer to indicate that its full
    temp = SPI1BUF;                 // Store whatever has been put in the receive buffer

    SPI1BUF = sram_mode;            // Set mode of operation
    while (!SPI1STATbits.SPIRBF);   // Wait for receive buffer to indicate that its full
    temp = SPI1BUF;                 // Store whatever has been put in the receive buffer

    SRAM_CS = 1;                    // Disable SRAM
    
    sram_fill(0);                   // fill memory with zeros to avoid initial noise
}

void sram_write(unsigned long sramaddress, unsigned int sramdata) 
{
    unsigned char temp;
    
    SRAM_CS = 0;                    // Enable SRAM

    SPI1BUF = WRITE;                // Send WRITE command, to be followed by 24-bit address
    while (!SPI1STATbits.SPIRBF);   // Wait for receive buffer to indicate that its full
    temp = SPI1BUF;                 // Store whatever has been put in the receive buffer

    SPI1BUF = sramaddress >> 16;    // Send first 8 bits of address (16->23)
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF;

    SPI1BUF = sramaddress >> 8;     // Send second 8 bits of address (8->15)
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF;

    SPI1BUF = sramaddress;          // Send third 8 bits of address (0->7)
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF;
    
    SPI1BUF = sramdata >> 8;        // send upper byte
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF;

    SPI1BUF = sramdata;             // Send lower byte
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF;

    SRAM_CS = 1;                    // Disable SRAM

}

unsigned int sram_read(unsigned long sramaddress) 
{
    unsigned int temp;

    SRAM_CS = 0;                    // Enable SRAM

    SPI1BUF = READ;                 // Send WRITE command, to be followed by 24-bit address
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF;

    SPI1BUF = sramaddress >> 16;    // Send first 8 bits of address (16->23)
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF;

    SPI1BUF = sramaddress >> 8;     // Send second 8 bits of address (8->15)
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF;

    SPI1BUF = sramaddress;          // Send third 8 bits of address (0->7)
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF;

    SPI1BUF = 255;
    while (!SPI1STATbits.SPIRBF);
    temp = SPI1BUF << 8;            // receive upper byte

    SPI1BUF = 255;
    while (!SPI1STATbits.SPIRBF);
    temp = temp | SPI1BUF;          // recieve lower byte and add it to the upper.

    SRAM_CS = 1;                    // Disable SRAM

    return (temp);                  // Return the received data

}

void initLCD(void) //Initialize display
 {
    //SET ALL AS OUTPUT
    TRISBbits.TRISB8 = 0;
    TRISBbits.TRISB9 = 0;
    TRISBbits.TRISB10 = 0;
    TRISBbits.TRISB11 = 0;
    TRISBbits.TRISB12 = 0;
    TRISBbits.TRISB13 = 0;
    
    // SET ALL LOW INITIALLY
    LCD_D4 = 0;
    LCD_D5 = 0;
    LCD_D6 = 0;
    LCD_D7 = 0;
    LCD_EN = 0;
    LCD_RS = 0;
    
    __delay_ms(15);         // wait 15ms for Vcc to rise
    LCD_lowerNibble(0x03);  // (upper 4 bits dont matter)
    LCD_pulse();
    __delay_ms(5);          // wait 5ms
    LCD_pulse();
    __delay_us(100);        // wait 100us
    LCD_pulse();
    LCD_lowerNibble(0x02); // send command to put into 4 bit mode (upper 4 bits dont matter)
    LCD_pulse();
    LCD_cmd(0x2C);         // function set (all lines, 5x7 characters)
    LCD_cmd(0x0C);         // display ON, cursor off, no blink
    LCD_cmd(0x01);         // clear display
    LCD_cmd(0x06);         // entry mode set, increment & scroll left
 }

void LCD_setPosition(unsigned int c)
 {
    LCD_upperNibble(c | 0x80); // send high nibble (bit 7 must be high for setting DDRam ADDR)
    LCD_pulse();
    LCD_lowerNibble(c);        // send low nibble
    LCD_pulse();
 }

void LCD_cmd( unsigned int c)
 {
    LCD_upperNibble(c);       // send high nibble
    LCD_pulse();
    LCD_lowerNibble(c);       // send low nibble
    LCD_pulse();
 }

void LCD_pulse(void)
 {
    LCD_EN = 1;
    __delay_ms(1);
    LCD_EN =0;
    __delay_ms(1);
 }

void LCD_upperNibble(unsigned int c)
{
    if(c & 0x80) LCD_D7=1; else LCD_D7=0;
    if(c & 0x40) LCD_D6=1; else LCD_D6=0;
    if(c & 0x20) LCD_D5=1; else LCD_D5=0;
    if(c & 0x10) LCD_D4=1; else LCD_D4=0;
}

void LCD_lowerNibble(unsigned int c)
{
    if(c & 0x08) LCD_D7=1; else LCD_D7=0;
    if(c & 0x04) LCD_D6=1; else LCD_D6=0;
    if(c & 0x02) LCD_D5=1; else LCD_D5=0;
    if(c & 0x01) LCD_D4=1; else LCD_D4=0;
}

void LCD_putChar(unsigned int c)
 {
    LCD_RS = 1;
    LCD_upperNibble(c); // send high nibble
    LCD_pulse();
    LCD_lowerNibble(c); // send low nibble
    LCD_pulse();
    LCD_RS = 0;
 }

void LCD_putString(char *buffer)
{
    while(*buffer != '\0')    // note: last el of array should be \0
    {      
        LCD_putChar(*buffer); // write character
        buffer++;             // go to next character
    }
}

void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void)
{
    IFS0bits.U1TXIF = 0; // clear TX interrupt flag
}

void updateLCD(void)
{
    // Strings to hold parameters to display on LCD
    char dist_param[18] = {};       // Stores "Th:[value]"
    char trem_param[18] = {};       // Stores "Speed:[value]Hz"
    char del_time_param[18] = {};   // Stores "Time:[value]ms        
    char del_decay_param[18] = {};  // Stores "Dec:[value]%"
    char chorus_param[18] = {};     // Stores "Speed:[value]s"
    
    LCD_clear();    // first clear display
    switch(MODE)
    {
        case 1: // MODE 1: Clean (no effect)
            LCD_line1();
            LCD_putString("Clean\0");
            LCD_line2();
            LCD_putString("No effect\0");
            break;

        case 2: // MODE 2: Distortion
            sprintf(dist_param, "Th:%d\0", dist_th); // Converts value to be part of string
            
            LCD_line1();
            LCD_putString("Distortion\0");
            LCD_line2();
            LCD_putString(dist_param);
            LCD_putString("% \0");
            LCD_putString("Type:\0");
            if (symmetric == 0)
                LCD_putString("Asym\0");
            else
                LCD_putString("Sym\0");
            break;

        case 3: // MODE 3: Tremolo
            sprintf(trem_param, "Speed:%.1fHz\0", trem_speed); // Converts value to be part of string
            
            LCD_line1();
            LCD_putString("Tremolo\0");
            LCD_line2();
            LCD_putString(trem_param);
            break;

        case 4: // MODE 4: Delay
            sprintf(del_time_param, "Time:%dms", delay_time); // Converts value to be part of string
            sprintf(del_decay_param, "Dec:%.1f\0", delay_decay); // Converts value to be part of string

            LCD_line1();
            LCD_putString("Delay  \0");
            LCD_putString(del_decay_param);
            LCD_putString("%\0");
            LCD_line2();
            LCD_putString(del_time_param);
            break;

        case 5: // MODE 5: Chorus
            sprintf(chorus_param, "Speed:%.2fs\0", chorus_speed); // Converts value to be part of string
            
            LCD_line1();
            LCD_putString("Chorus\0");
            LCD_line2();
            LCD_putString(chorus_param);
            break;

        default: // Error - unexpected MODE
            LCD_line1();
            LCD_putString("Error\0");
            LCD_line2();
            LCD_putString("Unknown Mode\0");
            break;
    }
}

void applyCom(void)
{
    LED = 0;    // turn rate LED off
    
    unsigned int msg;
    sscanf(message, "%d", &msg);    // convert from char array to actual integer
    
    if (identifier == 'm')  // set the effect mode
    {
        switch ( msg )
        {                              
            case    1:    //CLEAN
                MODE = 1;
                break;
            case    2:    //DISTORTION
                MODE = 2;
                sram_fill(0); // fill memory with zeros
                break;
            case    3:    //TREMOLO
                MODE = 3;
                break;
            case    4:    //DELAY
                MODE = 4;
                break;
            case 5: // CHORUS
                MODE = 5;
                break;
            default:
                MODE = 1;
                break;
        }
    }
    
    else if (identifier == 'f') // set the trem frequency
    {
        int maxFreq = 10;
        msg = msg+1;                                // 0->99 becomes 1->100
        float tremfreq = (msg * 0.01 * maxFreq);    // received is a percentage so scale accordingly (max 15Hz)
        trem_speed = tremfreq;                      // store to display on LCD
        tremPeriod = (Fs/(tremfreq*31))-1;
    }
    
    else if (identifier == 'd') // set the delay decay
    {
        delay_decay = (100-(msg*6.25));  // save percentage to be displayed on lcd
        delayVol = msg;                  // Set decay volume
    }
    
    else if (identifier == 't') // set the delay time
    {
        msg = msg+1;                        // 0->99 becomes 1->100
        unsigned long tmp = 655.36 * msg;
        unsigned long delayTime = tmp*2;   // double the result since we store each sample as 2 bytes
        
        delay_time = 1482*(1-(msg*0.01)); // save time to be displayed on lcd

        write_address = 0;                          // reset write address
        read_address = write_address + delayTime;   // set read according to delay time

    }
    
    else if (identifier == 'g') // set the distortion threshold
    {
        msg = msg+1;                        // 0->99 becomes 1->100
        dist_th = msg;                      // save percentage to be displayed on lcd
        msg = 100-msg;                      // invert percentage
        float threshold = 0.0035 * msg;     // calculate threshold
        Pthreshold = Q15(threshold);        // set upper limit
        Nthreshold = Q15(threshold*(-1));   // set lower limit
    }
    
    else if (identifier == 'p') // set the chorus speed
    {
        chorusPeriod = (Fs/340) * (1 + (0.01*(msg+1))); // set chorus speed using period
        chorus_speed = 2*(1 + (0.01*(msg+1))); // save speed to be displayed on lcd
        // equation gives a speed of between 2 and 4 seconds
    }
    
    else if (identifier == 's') // set the distortion type
    {
        symmetric = msg;
    }
    
    else    // unrecognised identifier
    {
        //we have a problem
    }
    
    updateLCD();    // update the LCD since things have changed
}


void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void) 
{ 
    unsigned char RxBuff = '\0';    // Holds received UART value
    
    if ( IFS0bits.U1RXIF  ==  1 )   // If UART interrupt flag...
    {
        RxBuff           =  U1RXREG;    // store UART buffer contents
        IFS0bits.U1RXIF  =  0;          // reset interrupt flag
        
        if (RxBuff == '#')   // if we receive this, transmission has ended
        {
            // run our applyCom to apply the received parameters
            applyCom();
            // then clear all variables ready for the next transmission
            identifier = '\0';
            message[2] = "";
            msgCounter = 0;
        }
        else    // otherwise we have received useful information so...
        {
            if(identifier == '\0')   // if the identifier is not yet set, that is what we have received
            {
                identifier = RxBuff;    // set accordingly
            }
            else    // otherwise we already have the identifier and we are receiving data
            {
                message[msgCounter] = RxBuff;
                msgCounter++;   // increment counter (since data may be of variable length)
            }
        }


    }
} 

signed int distortion(signed int dist_in)
{
    if (dist_in > Pthreshold)
        return Pthreshold;
    else if (dist_in < Nthreshold)
        if (symmetric==1)
            return Nthreshold;
        else
            return dist_in;
    else
        return dist_in;
}

signed int delay(signed int delay_in)
{
    signed int delay_out;
    
    delay_in = delay_in >> 1;   // attenuate sample to avoid clipping
        
    if (read_address==131072)   // Check if max memory location has been reached
        read_address = 0;       // wrap around

    if (write_address==131072)  // Check if max memory location has been reached
        write_address = 0;      // wrap around

    delay_out = sram_read(read_address);   // Read delayed sample from memory
    read_address = read_address + 2;       // increment address pointer
    
    // attenuate delayed sample according to decay factor and add to current sample
    delay_out = volume(delay_out, delayVol) + delay_in;    
    
    sram_write(write_address, delay_out);   // write the new combined sample to memory
    write_address = write_address + 2;      // increment address pointer
    
    return delay_out;
}

signed int tremolo(signed int trem_in)
{
    static int LFO = 0;
    static int tremVol = 8;      // Volume variable (0: 0% volume, 8: 100% volume)
    static int direction = 0;  
    
    if (LFO == tremPeriod)   // when max LFO is reached (according to trem frequency)
    {
        if (tremVol >= 16)      //Max volume reached
            direction = 0;
        else if (tremVol <= 0)  // min volume reached
            direction = 1;

        if (direction == 1)
        {
            LED = 1;
            tremVol++;
        }
        else
        {
            LED = 0;
            tremVol--;
        }
        LFO = 0;    // reset LFO
    }
    else
        LFO++;
    
    trem_in = volume(trem_in, tremVol); // Attenuate sample according to current volume value
    trem_in = lowpass(trem_in);   // Basic lpf to reduce high freq amplitude transient noise
    
    return trem_in;
}

signed int chorus(signed int chorus_in)
{
    #define SIZE 340   // Short sample buffer

    static signed int buf[SIZE];
    static int write;
    static int read;
    static int delay_len;
    static int delay_dir;
    static int LFO = 0;
    
    signed int sample_out;  
    
    chorus_in = chorus_in >> 1; // halve the sample amplitude to avoid clipping

    read = write - delay_len;   // set read address according to current delay time

    buf[write] = chorus_in;    // store the current sample 
    if (write > SIZE-1)     // if the final memory location is reached...
        write = 0;          // wrap around
    else
        write++;            // otherwise increment the write pointer

    if (read < 0)   // sometimes the read pointer will be negative, if this is the case
        read = read + SIZE; // add the buffer length to the read pointer to get the correct memory location

    sample_out = buf[read];    // retrieve the sample and half the amplitude
    read++;                         // increment the read pointer

    if(LFO == chorusPeriod)  // LFO value triggers a change in delay length (currently fixed))
    {
        if (delay_dir == 0) // delay length is increasing
        {
            LED = 0; 
            delay_len++;
        }
        else    // delay length is decreasing
        {
            LED = 1;
            delay_len--;
        } 
        LFO = 0;  // reset LFO
    }

    LFO++;  // Increment the LFO counter (this is incremented at the sample rate)

    if (delay_len >= SIZE-1)    // if the delay length reaches the buffer length
        delay_dir = 1;          // set the delay length to begin decreasing
    if (delay_len <= 1)         // if the delay length reaches a very small value 
        delay_dir = 0;          // set the delay length to begin increasing 

    sample_out = (sample_out>>1) + chorus_in; // add the delayed sample (attenuated) to the current sample

    return sample_out;
}

signed int lowpass(signed int lp_in) 
{
    static signed long lp_out = 0;  // variable persists from one call of a function to another

    lp_out = ((((lp_in - lp_out) * 8000) >> 15) + lp_out);   // shift by 15 to divide by approximate sampling freq

    return (lp_out);
}