/*
 * main.c
 *
 *  Created on: Sep 9, 2024
 *      Author: Salma Youssef
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

unsigned char secs = 0, mins = 0, hrs = 0;
unsigned char IsPaused = 0;
unsigned char IsCountDown = 0;

void Timer1_init() {
	TCNT1 = 0;
	TCCR1A = (1 << FOC1A) | (1 << FOC1B);
	TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);
	OCR1A = 15625;
	TIMSK |= (1 << OCIE1A);
}

void Buzzer_init() {
	DDRD |= (1 << PD0);               // output pin
}

void Hrs_init() {
	DDRB &= ~(1 << PB0);              // PB0 input
	PORTB |= (1 << PB0);              // pull-up
	DDRB &= ~(1 << PB1);
	PORTB |= (1 << PB1); //Pull up
}

void Mins_init() {
	DDRB &= ~(1 << PB4);
	PORTB |= (1 << PB4); // Pull up
	DDRB &= ~(1 << PB3);
	PORTB |= (1 << PB3);             // pull-up PB1
}

void Secs_init() {
	DDRB &= ~(1 << PB6);
	PORTB |= (1 << PB6); // Pull up
	DDRB &= ~(1 << PB5);
	PORTB |= (1 << PB5);              //pull-up PB2
}

void INT0_init() {
	/* Pull up
	 * Falling Edge*/
	DDRD &= ~(1 << PD2);
	PORTD |= (1 << PD2);
	MCUCR |= (1 << ISC01);
	MCUCR &= ~(1 << ISC00);
	GICR |= (1 << INT0);
}

void INT1_init() {
	/* Pull down
	 * Raising Edge*/
	DDRD &= ~(1 << PD3);
	MCUCR |= (1 << ISC11) | (1 << ISC10);
	GICR |= (1 << INT1);
	TCNT1 = 0;
	PORTD &= ~(1 << PD0);
}

void INT2_init() {
	/* Pull up
	 * Falling Edge*/
	DDRB &= ~(1 << PB2);
	PORTB |= (1 << PB2);
	MCUCSR &= ~(1 << ISC2);
	GICR |= (1 << INT2);
}
void Switch_mode(void) {
	DDRB &= ~(1 << PB7);
	PORTB |= (1 << PB7);
}
ISR(TIMER1_COMPA_vect) {

	if (IsCountDown) {
		if (secs == 0) {
			if (mins == 0) {
				if (hrs == 0) {
					//Turn on the buzzer
					PORTD |= (1 << PD0);
					return;
				}
				hrs--;
				mins = 59;
				secs = 59;
			} else {
				mins--;
				secs = 59;
			}
		} else {
			secs--;
		}
	}
//Count Upwards mode
	else {
		if (!(PINB & (1 << PB7))) {
			//Turn on Red led
			PORTD |= (1 << PD4);
			//Turn off buzzer
			PORTD &= ~(1 << PD0);
		}
		secs++;
		if (secs == 60) {
			secs = 0;
			mins++;
			if (mins == 60) {
				mins = 0;
				hrs++;
				if (hrs == 24) {
					hrs = 0;
					mins = 0;
					secs = 0;
				}
			}
		}
	}
}

ISR(INT0_vect) {
	TCNT1 = 0;
	hrs = 0;
	mins = 0;
	secs = 0;
}

ISR(INT1_vect) {
	TCCR1B = 0xF8;
	IsPaused = 1;
}

ISR(INT2_vect) {
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
	PORTD &= ~(1 << PD0);
	IsPaused = 0;
}
void Segment_output() {
//	/*
//	 * PA0-PA5 is an enable pin so we can display each digit really fast, so will seem smooth and this how we control multiplexed 6 seven segment
//	 */
	PORTA = 0x20;
	PORTC = (PORTC & 0xF0) | (secs % 10 & 0x0F);
	_delay_ms(2);
	PORTA = 0x10;
	PORTC = (PORTC & 0xF0) | (secs / 10 & 0x0F);
	_delay_ms(2);
	PORTA = 0x08;
	PORTC = (PORTC & 0xF0) | (mins % 10 & 0x0F);
	_delay_ms(2);
	PORTA = 0x04;
	PORTC = (PORTC & 0xF0) | (mins / 10 & 0x0F);
	_delay_ms(2);
	PORTA = 0x02;
	PORTC = (PORTC & 0xF0) | (hrs % 10 & 0x0F);
	_delay_ms(2);
	PORTA = 0x01;
	PORTC = (PORTC & 0xF0) | (hrs / 10 & 0x0F);
	_delay_ms(2);
}
void LEDS_init(void) {
	DDRD |= (1 << PD4); //RED
	DDRD |= (1 << PD5); //YELLOW
}
int main() {
	INT0_init();             // pause button
	INT1_init();             // resume button
	INT2_init();

	Hrs_init();
	Mins_init();
	Secs_init();             // reset button

	Buzzer_init();

	Switch_mode();
	LEDS_init();

	PORTD |= (1 << PD4); // Turn on red turn yellow
	PORTD &= ~(1 << PD5);

	DDRC = 0x0F; // PC0-PC3
	PORTC = 0xF0;
	DDRA = 0x3F; // PA0-PA6 to enable digits

	Timer1_init();

	SREG |= (1 << 7); // Enable global interrupts

	while (1) {
		Segment_output();
		/*
		 * If stopwatch is paused and the count mode button is clicked,
		 * then both red and yellow led are toggled. and of course toggle iscountdown flag.
		 */
		while (IsPaused) {
			Segment_output();
			if (!(PINB & (1 << PB7))) {
				IsCountDown ^= 1;
				//Turn on yellow led
				PORTD ^= (1 << PD5);
				//Turn off red led
				PORTD ^= (1 << PD4);
				while (!(PINB & (1 << PB7))) {
					Segment_output();
				}
			}
			//Hours increment button
			if (!(PINB & (1 << PB1))) {
				if (hrs == 24) {
					hrs = 24;
				} else {
					hrs++;
				}
				while (!(PINB & (1 << PB1))) {
					Segment_output();
				}
			}

			//Hours decrement button
			if (!(PINB & (1 << PB0))) {
				if (hrs == 0) {
					hrs = 0;
				} else {
					hrs--;
				}
				while (!(PINB & (1 << PB0))) {
					Segment_output();
				}
			}

			//minutes increment button
			if (!(PINB & (1 << PB4))) {
				if (mins == 59) {
					mins = 59;
				} else {
					mins++;
				}
				while (!(PINB & (1 << PB4))) {
					Segment_output();
				}
			}

			// minutes decrement button
			if (!(PINB & (1 << PB3))) {
				if (mins == 0) {
					mins = 0;
				} else {
					mins--;
				}
				while (!(PINB & (1 << PB3))) {
					Segment_output();
				}
			}

			//seconds increment button
			if (!(PINB & (1 << PB6))) {
				if (secs == 59) {
					secs = 59;
				} else {
					secs++;
				}
				while (!(PINB & (1 << PB6))) {
					Segment_output();
				}
			}

			// seconds decrement button
			if (!(PINB & (1 << PB5))) {
				if (secs == 0) {
					secs = 0;
				} else {
					secs--;
				}
				while (!(PINB & (1 << PB5))) {
					Segment_output();
				}
			}

		}
	}
}

