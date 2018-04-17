#include "multifx.h"

#include <libpic30.h>

#include "p33fxxxx.h"
#include <xc.h>

#include <stdio.h>
#include <stdbool.h>
#include <dsp.h>
#include "Serial.h"
#include "Audio.h"
#include "Delay.h"
#include "Distortion.h"
#include "Tremolo.h"
#include "Chorus.h"




// Bluetooth communication storage
unsigned char identifier = '\0';    // character that indicated operation to perform
unsigned char message[2] = "";      // message holds data, usually as percentage
unsigned int  msgCounter = 0;       // counter for received message

// Parameters for various effects, later converted to strings for LCD
int   dist_th = 50;     // threshold as a percentage
float trem_speed = 5;   // tremolo speed in Hz
int   delay_time = 741; // delay time is seconds
float delay_decay = 50; // delay decay as percentage
float chorus_speed = 3; // chorus speed in Hz

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

void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void)
{
    IFS0bits.U1TXIF = 0; // clear TX interrupt flag
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

void applyCom(void)
{
    LED = 0;    // turn rate LED off
    
    unsigned int msg;
    sscanf(message, "%d", &msg);    // convert from char array to actual integer
    
    mode_t mode;
    
    if (identifier == 'm')  // set the effect mode
    {
        mode = (mode_t)msg; // cast to mode_t enum - TODO: this may not work
        setMode(mode);
        /*
        switch ( msg )
        {                              
            case    1:    //CLEAN
                mode = 1;
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
        */
    }
    
    else if (identifier == 'f') // set the trem frequency
    {
        int maxFreq = 10;
        msg = msg+1;                                // 0->99 becomes 1->100
        float tremfreq = (msg * 0.01 * maxFreq);    // received is a percentage so scale accordingly (max 15Hz)
        trem_speed = tremfreq;                      // store to display on LCD
        chorus_set_period( (Fs/(tremfreq*31))-1 );
    }
    
    else if (identifier == 'd') // set the delay decay
    {
        delay_decay = (100-(msg*6.25));  // save percentage to be displayed on lcd
        delay_set_decay(msg);                  // Set decay volume
    }
    
    else if (identifier == 't') // set the delay time
    {
        msg = msg+1;                        // 0->99 becomes 1->100
        unsigned long tmp = 655.36 * msg;
        unsigned long delayTime = tmp*2;   // double the result since we store each sample as 2 bytes
        
        delay_time = 1482*(1-(msg*0.01)); // save time to be displayed on lcd

        delay_set_delay_time(delayTime);   // set read according to delay time

    }
    
    else if (identifier == 'g') // set the distortion threshold
    {
        msg = msg+1;                        // 0->99 becomes 1->100
        dist_th = msg;                      // save percentage to be displayed on lcd
        msg = 100-msg;                      // invert percentage
        float threshold = 0.0035 * msg;     // calculate threshold
        distortion_set_positive_cutoff(threshold);
        distortion_set_positive_cutoff(threshold*(-1));
    }
    
    else if (identifier == 'p') // set the chorus speed
    {
        int chorus_period = (Fs/340) * (1 + (0.01*(msg+1))); // set chorus speed using period
        chorus_set_period(chorus_period);
        //TODO - sort out below
        chorus_speed = 2*(1 + (0.01*(msg+1))); // save speed to be displayed on lcd
        // equation gives a speed of between 2 and 4 seconds
    }
    
    else if (identifier == 's') // set the distortion type
    {
        if (msg == 0)
            distortion_set_symetric(false);
        else
            distortion_set_symetric(true);
    }
    
    else    // unrecognised identifier
    {
        //we have a problem
    }
    
    updateLCD();    // update the LCD since things have changed
}