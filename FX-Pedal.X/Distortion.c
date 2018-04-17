#include "Distortion.h"
#include <xc.h>
#include <libpic30.h>
#include <stdio.h>
#include <stdbool.h>
#include <dsp.h>

// Distortion thresholds
fractional Pthreshold = Q15(0.175);   // +ve Cutoff Converted to Q15
fractional Nthreshold = Q15(-0.175);  // -ve Cutoff Converted to Q15
int symmetric = 0;                    // flag to indicate whether clipping is symmetric or asymmetric

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

void distortion_set_positive_cutoff(float threshold)
{
    Pthreshold = Q15(threshold);        // set upper limit
}

void distortion_set_negative_cutoff(float threshold)
{
    Nthreshold = Q15(threshold);   // set lower limit
}

void distortion_set_symetric(bool is_symetric)
{
    if (is_symetric)
        symmetric = 1;
    else
        symmetric = 0; 
}