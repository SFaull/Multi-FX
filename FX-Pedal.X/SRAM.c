/* Driver for the SRAM chip */
#include "SRAM.h"

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
    SPI1CON1bits.CKE = 0;       //data changes on falling edge (active to idle)
    SPI1CON1bits.CKP = 0;       //idle is low
    SPI1CON1bits.PPRE = 3;      //1:1 primary prescaler
    SPI1CON1bits.SPRE = 6;      //2:1 secondary prescaler (20Mhz SPI clock)
    SPI1CON1bits.MSTEN = 1;     //master mode
    SPI1CON1bits.SMP = 0;       //data sampled in middle (Input data is sampled at the middle of data output time)
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