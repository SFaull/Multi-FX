#include "Delay.h"
#include "SRAM.h"
#include "Volume.h"

int delayVol = 8;       // default volume for delay decay (8/16 -> 50%)

// SRAM address pointers
unsigned long write_address = 0;
unsigned long read_address = 65536;   // half of maximum value.. this is simply the default value

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

void delay_set_decay(int decay)
{
    delayVol = decay;
}

void delay_set_delay_time(int delayTime)
{
    write_address = 0;                          // reset write address
    read_address = write_address + delayTime;   // set read according to delay time
}