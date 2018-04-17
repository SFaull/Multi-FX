#include "LCD.h"
#include "p33fxxxx.h"
#include "multifx.h"
#include <libpic30.h>
#include <stdio.h>

// Parameters for various effects, later converted to strings for LCD
extern int   dist_th;     // threshold as a percentage
extern float trem_speed;   // tremolo speed in Hz
extern int   delay_time; // delay time is seconds
extern float delay_decay; // delay decay as percentage
extern float chorus_speed; // chorus speed in Hz
extern int symmetric;                    // flag to indicate whether clipping is symmetric or asymmetric

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


void updateLCD(void)
{
    // Strings to hold parameters to display on LCD
    char dist_param[18] = {};       // Stores "Th:[value]"
    char trem_param[18] = {};       // Stores "Speed:[value]Hz"
    char del_time_param[18] = {};   // Stores "Time:[value]ms        
    char del_decay_param[18] = {};  // Stores "Dec:[value]%"
    char chorus_param[18] = {};     // Stores "Speed:[value]s"
    
    LCD_clear();    // first clear display
    mode_t mode = getMode();
    
    switch(mode)
    {
        case kMode_clean: // MODE 1: Clean (no effect)
            LCD_line1();
            LCD_putString("Clean\0");
            LCD_line2();
            LCD_putString("No effect\0");
            break;

        case kMode_distortion: // MODE 2: Distortion
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

        case kMode_tremolo: // MODE 3: Tremolo
            sprintf(trem_param, "Speed:%.1fHz\0", trem_speed); // Converts value to be part of string
            
            LCD_line1();
            LCD_putString("Tremolo\0");
            LCD_line2();
            LCD_putString(trem_param);
            break;

        case kMode_delay: // MODE 4: Delay
            sprintf(del_time_param, "Time:%dms", delay_time); // Converts value to be part of string
            sprintf(del_decay_param, "Dec:%.1f\0", delay_decay); // Converts value to be part of string

            LCD_line1();
            LCD_putString("Delay  \0");
            LCD_putString(del_decay_param);
            LCD_putString("%\0");
            LCD_line2();
            LCD_putString(del_time_param);
            break;

        case kMode_chorus: // MODE 5: Chorus
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