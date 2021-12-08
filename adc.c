#include <avr/io.h>

#include "adc.h"

void adc_init(void)
{
    // Initialize the ADC
    ADMUX |= ( (1 << REFS0) | (1 << ADLAR) ); // choose AVCC as the high reference and set results to 8-bit
    ADCSRA |= ( (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) ); // turn on enable and set prescalar to 128
}

unsigned char adc_sample(unsigned char channel)
{
    ADMUX |= channel; // set ADC input mux bits to 'channel' value
    ADCSRA |= (1 << ADSC); // set the "start" bit to 1

    // Convert an analog input and return the 8-bit result
    while( (ADCSRA & (1 << ADSC)) != 0 ){
        // while the ADC is taking the sample
    }
    unsigned char result = ADCH;
    return result;
}
