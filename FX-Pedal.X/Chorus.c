#include "Chorus.h"
#include "multifx.h"
#include "Audio.h"
#include <stdbool.h>

int chorusPeriod = 195; // chorus period used to determine the frequency (default value)
float chorusFreq;

int chorus_get_period(void) { return chorusPeriod; }
float chorus_get_freq(void) { return chorusFreq; }

void chorus_set_period(int percentage) 
{ 
        chorusPeriod = (Fs/340) * (1 + (0.01*percentage)); // set chorus speed using period
        //TODO - sort out below and explain magic numbers
        chorusFreq = 2*(1 + (0.01*percentage)); // save speed to be displayed on lcd
        // equation gives a speed of between 2 and 4 seconds
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
            rate_led_enabled(false);
            delay_len++;
        }
        else    // delay length is decreasing
        {
            rate_led_enabled(true);
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

signed int test(signed int sample)
{
    
    /*
    unsigned int lost_bit = rand() % 15;
    signed int mask = (0b0000000000000001 << lost_bit);
    mask = ~mask;
    return (sample & mask);
     */
    

    return (sample & 0b0101010101010101);

    
    /*
    return (sample & 0b1010101010101010);
    */
    
    /*
    signed int temp = sample << 4;
    return (temp >> 4);
    */
    
    
    /*
    static unsigned int count = 0;
    static unsigned int limit = 2;
    
    if (count == limit)
    {
        count = 0;
        // random int between 0 and 3 
        limit = rand() % 2;
        return sample;
    }
    else
    {
        count++;
        return 0;
    }
    */
}