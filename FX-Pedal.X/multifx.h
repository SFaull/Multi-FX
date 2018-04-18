// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef MULTIFX_H
#define	MULTIFX_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdbool.h>

/* Definitions */
#define FOSC		79227500                // Clock-frequecy in Hz -> Fin*(M/(N1*N2)) = 7.37*(43/(2*2)) = 79227500 
#define FCY      	(FOSC/2)                // MCU is running at FCY MIPS (must be defined correctly to use "__delay_ms()" func)

#define LED      LATBbits.LATB4     // Rate LED

//#define delay_ms(x) __delay_ms(x)

typedef enum {
    kMode_clean = 0,
    kMode_distortion = 1,
    kMode_tremolo = 2,
    kMode_delay = 3,
    kMode_chorus = 4
} mode_t;

/* Function declarations */
// Initialisations
void initOsc(void);
void initIO(void);
void rate_led_enabled(bool state);
void set_fx_mode(mode_t mode);
mode_t get_fx_mode(void);


#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    // TODO If C++ is being used, regular C code needs function names to have C 
    // linkage so the functions can be used by the c code. 

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* XC_HEADER_TEMPLATE_H */