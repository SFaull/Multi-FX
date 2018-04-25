#include "Encoder.h"
#include "LCD.h"
#include "multifx.h"
#include <stdbool.h>

void knobTurned(int L, int R)
{    
    static int state = 0;           // will store two bits for pins A & B on the encoder which we will get from the pins above
    static int bump[] = {0,0,1,-1};
    
    mode_t current_mode = get_fx_mode();
    int modeInt = (int)current_mode;    // cast to int
    
    state = 0;          // reset each time
    state = state + L;  // add the state of Pin L
    state <<= 1;        // shift the bit over one spot
    state = state + R;  // add the state of Pin R
 
     // posible states:
     //00 & 01 - something is wrong, do nothing
     //10 - knob was turned forwards
     //11 - knob was turned backwards
     
     modeInt = modeInt + bump[state]; // Add direction from bump position to MODE
     
     if (modeInt >= (int)kMode_count)      // dont exceed maximum mode value
         modeInt = 0;      // loop round instead
     else if (modeInt < 0) // dont go lower than the minimum mode value
         modeInt = (int)kMode_count - 1;      // loop round instead
     
     set_fx_mode((mode_t)modeInt);

     rate_led_enabled(false);       // turn rate LED off
     updateLCD();   // update LCD because mode has changed

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