/********************************************
 *
 *  Name: Emily Kuo
 *  Email: ekuo@usc.edu
 *  Section: Fall 2020 CP
 *  Assignment: Project - Music Box
 *
 ********************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdio.h>

#include "lcd.h"
#include "adc.h"

#define NUM_NOTES 21
#define NUM_TONES 26

void play_note(unsigned int);
void display_notes(unsigned char);
void timer1_init(void);

// TIMER1 output connected to buzzer PB4
// Rotary encoder connected to PC1, PC5

/*
  The note_freq array contains the frequencies of the notes from C3 to C5 in
  array locations 1 to 25.  Location 0 is a zero for a note rest.
*/
unsigned int note_freq[NUM_TONES] =
{ 0,   131, 139, 147, 156, 165, 176, 185, 196, 208, 220, 233, 247,
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523};

char note_name[NUM_TONES][2] = 
{ "  ", "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B ",
  "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B ", "C "};

char note_octave[NUM_TONES] = 
{ ' ', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3', '3',
  '4', '4', '4', '4', '4', '4', '4', '4', '4', '4', '4', '4', '5'};

// raw inputs of rotary encoder
volatile unsigned char a, b;
volatile unsigned char x;

volatile unsigned char new_state, old_state;
volatile char note_change = 0;
volatile unsigned char state_change = 0;

volatile unsigned char note_signal = 0; // flag that changes every note frequency
volatile unsigned int timer_stop = 0; // flag that once 0.5 second is reached
unsigned char x_pos = 1;
unsigned char screen_num = 1;

/* Some sample tunes for testing */
/*
  The "notes" array is used to store the music that will be played.  Each value
  is an index into the tone value in the note_freq array.
*/

// G, A, F, F(octive lower), C     Close Encounters
//unsigned char notes[NUM_NOTES] = {20, 22, 18, 6, 13};

// D D B C A B G F# E D A C A B    Fight On
//unsigned char notes[NUM_NOTES] = {15, 15, 12, 13, 10, 12, 8, 7, 5, 3, 10, 13, 10, 12};

// E E F G G F E D C C D E E D D   Ode to Joy
unsigned char notes[NUM_NOTES] = {17, 17, 18, 20, 20, 18, 17, 15, 13, 13, 15, 17, 17, 15, 15};

int main(void) {
	// initialize various modules and functions
	lcd_init();
	adc_init();
	timer1_init();

	// set up ports and timer interrupts for the rotary encoder
	PORTC |= ( (1 << PC1) | (1 << PC5) );
	PCICR |= (1 << PCIE1); // enable the pin change interrupts on PORTC
	PCMSK1 |= ((1 << PCINT9) | (1 << PCINT13)); // enable interrupts for PC1 and PC5

	// set up ports for the buzzer
	DDRB |= (1 << PB4);
	PORTB |= (1 << PB4);

	// enable global interrupts
	sei();

	// show splash screen on the LCD display
	lcd_writecommand(1); // clear screen
	lcd_moveto(0, 3);
	lcd_stringout("Music Box");
	lcd_moveto(1, 3);
	lcd_stringout("Emily Kuo");
	_delay_ms(1000);
	lcd_writecommand(1); // clear screen

	// LCD button value (default = 255)
	// select = ~206, left = ~155, up = ~51, down = ~101, right = ~0
	display_notes(screen_num);
	lcd_moveto(0, x_pos);

	// read the rotary encoder's inputs
    x = PINC;
    a = x & (1 << PC1);
    b = x & (1 << PC5);

	// determine the intial state of the rotary encoder  
    if (!b && !a)
		old_state = 0;
    else if (!b && a)
		old_state = 1;
    else if (b && !a)
		old_state = 2;
    else
		old_state = 3;
    new_state = old_state;

	while (1) {
		unsigned char adc_result = adc_sample(0); // poll the LCD buttons
		if( adc_result < 215 ){ // if a button is pressed
			// if up or right button is pressed
			if ( (adc_result > 40 && adc_result < 60) || (adc_result < 10) ){
				if((x_pos == 13) && (screen_num + 1 <= 3)){
					screen_num += 1;
					x_pos = 1;
					display_notes(screen_num);
				}
				else if (x_pos + 2 <= 13) x_pos += 2;
				lcd_moveto(0, x_pos);
			}

			// if left or down button is pressed
			else if( (adc_result > 145 && adc_result < 165) || (adc_result > 90 && adc_result < 110) ){
				if((x_pos == 1) && (screen_num - 1 >= 1)){
					screen_num -= 1;
					x_pos = 13;
					display_notes(screen_num);
				}
				else if (x_pos - 2 >= 1) x_pos -= 2;
				lcd_moveto(0, x_pos);
			}

			// if select button is pressed
			else if( adc_result > 195 ){
				unsigned char i = 0;
				while(notes[i] != '\0'){
					unsigned int freq = note_freq[notes[i]];
					if(freq == 0) _delay_ms(500);
					else play_note(freq); // play the tune
					i++;
				}
			}
			_delay_ms(150); // handles button debouncing
		}

		// if rotary encoder was rotated, change note tone
		if (state_change) {
			state_change = 0; // reset state_change flag
			unsigned char index = (screen_num-1)*7 + x_pos/2; // get current note index
			if(notes[index] + note_change >= 0 && notes[index] + note_change < 26){
				notes[index] = notes[index] + note_change;
				note_change = 0;

				// display new notes on LCD screen
				char buf1[3];
				snprintf(buf1, 3, "%s", note_name[notes[index]]);
				lcd_stringout(buf1);
				lcd_moveto(1, x_pos);

				char buf2[2];
				snprintf(buf2, 2, "%c", note_octave[notes[index]]);
				lcd_stringout(buf2);
				lcd_moveto(0, x_pos);
			}
		}
	}
}

// show notes on the LCD screen
void display_notes(unsigned char screen){
	char first_line[17];
	char second_line[17];

	if(screen == 1){
		first_line[0] = ' ';
		first_line[15] = '>';
	}
	else if(screen == 2){
		first_line[0] = '<';
		first_line[15] = '>';
	}
	else if(screen == 3){
		first_line[0] = '<';
		first_line[15] = ' ';
	}
	second_line[0] = screen + '0';
	second_line[15] = ' ';

	unsigned char start = (screen-1)*7;
	unsigned char end = (screen)*7;

	for(unsigned char i = start; i < end; i++){
		unsigned char note_index = notes[i];
		unsigned char index  = (i-start)*2;
		first_line[index+1] = note_name[note_index][0];
		first_line[index+2] = note_name[note_index][1];
		second_line[index+1] = note_octave[note_index];
		second_line[index+2] = ' ';
	}

	lcd_writecommand(1); // clear the screen
	lcd_moveto(0,0);
	lcd_stringout(first_line);
	lcd_moveto(1,0);
	lcd_stringout(second_line);
}

// play a tone for 0.5 second at the frequency specified
void play_note(unsigned int freq){
	// make timer send a signal every half of the note's period
	OCR1A = 16*1000000/freq/64/2; // 16 MHz clock, prescalar = 64
	TCCR1B |= ((1 << CS11)|(1 << CS10)); // start the timer (set prescalar to 64)

	while(timer_stop <= freq){
		if(note_signal == 1){
			PORTB |= (1 << PB4); // make PB4 high
		}
		else if(note_signal == 2){
			PORTB &= ~(1 << PB4); // make PB4 low
			note_signal = 0;
		}
	}

	TCCR1B &= ~( (1 << CS12) | (1 << CS11) | (1 << CS10) ); // stop the timer
	timer_stop = 0;
}

// initialize timer for buzzer
void timer1_init(){
	// set to CTC (Clear Timer on Compare) mode - tells hardware to start over at 0
	// once the counter reaches the desired value
	TCCR1B |= (1 << WGM12);
	// enable timer interrupt
	TIMSK1 |= (1 << OCIE1A);
}

// timer interrupt for buzzer
ISR(TIMER1_COMPA_vect){
	note_signal++;
	timer_stop++;
}

// timer interrupt for rotary encoder
ISR(PCINT1_vect){
	// read the input bits and determine A and B
	x = PINC;
	a = x & (1 << PC1);
	b = x & (1 << PC5);

	// state machine
	if (old_state == 0) {
		if(a){
			new_state = 1;
			note_change--;
		}
		if(b){
			new_state = 2;
			note_change++;
		}
	}
	else if (old_state == 1) {
		if(b){
			new_state = 3;
			note_change--;
		}
		if(!a){
			new_state = 0;
			note_change++;
		}
	}
	else if (old_state == 2) {
		if(!b){
			new_state = 0;
			note_change--;
		}
		if(a){
			new_state = 3;
			note_change++;
		}
	}
	else { // old_state = 3
		if(!a){
			new_state = 2;
			note_change--;
		}
		if(!b){
			new_state = 1;
			note_change++;
		}
	}

	if (new_state != old_state) {
		state_change = 1;
		old_state = new_state;
	}
}