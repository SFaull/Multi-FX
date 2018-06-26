#include "Distortion.h"
#include <xc.h>
#include <libpic30.h>
#include <stdio.h>
#include <stdbool.h>
#include <dsp.h>

// Distortion thresholds
fractional Pthreshold = Q15(0.175);   // +ve Cutoff Converted to Q15
fractional Nthreshold = Q15(-0.175);  // -ve Cutoff Converted to Q15
static bool symmetric = 0;                    // flag to indicate whether clipping is symmetric or asymmetric
int distortionPercentage;

int distortion_get_percentage(void) { return distortionPercentage; }
bool distortion_get_symetric(void) { return symmetric; }
float distortion_get_negative_cutoff(void) { return Nthreshold; }
float distortion_get_positive_cutoff(void) { return Pthreshold; }



void distortion_set_percentage(int percentage)
{
    distortionPercentage = percentage;                      // save percentage to be displayed on lcd
    float threshold = 0.0035 * (100-distortionPercentage);     // calculate threshold using inverted percentage
    distortion_set_positive_cutoff(threshold);
    distortion_set_negative_cutoff(threshold*(-1));    
}

void distortion_set_positive_cutoff(float threshold)
{
    Pthreshold = Float2Fract(threshold);        // set upper limit
}

void distortion_set_negative_cutoff(float threshold)
{
    Nthreshold = Float2Fract(threshold);   // set lower limit
}

void distortion_set_symetric(bool is_symetric)
{
    symmetric = is_symetric;
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