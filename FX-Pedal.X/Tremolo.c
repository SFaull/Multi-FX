#include "Tremolo.h"
#include "multifx.h"
#include "Volume.h"
#include "Filter.h"

int tremPeriod = 200;   // tremolo period used to determine the frequency (default value)


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



void tremolo_set_period(int period)
{
    tremPeriod = period;
}