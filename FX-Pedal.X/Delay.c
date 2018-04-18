#include "Delay.h"
#include "SRAM.h"
#include "Volume.h"

#define MEMORY_SIZE 131072

int delayVol = 8;       // default volume for delay decay (8/16 -> 50%)
int delayTime = 741;          // delay time in

// SRAM address pointers
static unsigned long write_address = 0;
static unsigned long read_address = MEMORY_SIZE/2;   // half of maximum value.. this is simply the default value

float delay_get_decay(void) 
{ 
    float percentage = 100-(delayVol*6.25);
    return percentage; 
}

int delay_get_delay_time(void) { return delayTime; }    // returns delaytime in seconds

void delay_set_decay(int decay)
{
    delayVol = decay;
}

void delay_set_delay_time(int percentage)
{
    // TODO define these magic numbers and optimise
    
    unsigned long tmp = 655.36 * percentage;
    unsigned long delayTime = tmp*2;   // double the result since we store each sample as 2 bytes
        
    delayTime = 1482*(1-(percentage*0.01)); // save time (in seconds) to be displayed on lcd
    
    write_address = 0;                          // reset write address
    read_address = write_address + delayTime;   // set read according to delay time
}

signed int delay(signed int delay_in)
{
    signed int delay_out;
    
    delay_in = delay_in >> 1;   // attenuate sample to avoid clipping
        
    if (read_address == MEMORY_SIZE)   // Check if max memory location has been reached
        read_address = 0;       // wrap around

    if (write_address == MEMORY_SIZE)  // Check if max memory location has been reached
        write_address = 0;      // wrap around

    delay_out = sram_read(read_address);   // Read delayed sample from memory
    read_address = read_address + 2;       // increment address pointer
    
    // attenuate delayed sample according to decay factor and add to current sample
    delay_out = volume(delay_out, delayVol) + delay_in;    
    
    sram_write(write_address, delay_out);   // write the new combined sample to memory
    write_address = write_address + 2;      // increment address pointer
    
    return delay_out;
}