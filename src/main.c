/*
 * main.c
 *
 *  Created on: Oct 10, 2018
 *      Author: Luís Sousa, Leonor Santos
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#define TRUE 1
#define FALSE 0

#define GNS PB5
#define YNS PB4
#define RNS PB3
#define GEW PB2
#define YEW PB1
#define REW PB0

#define EMG_BUTTON PD2

#define RED_DELAY 500
#define YELLOW_DELAY 500
#define GREEN_DELAY 1000
#define BREAKDOWN_DELAY 1000
#define STOPPED_DELAY 3000

#define BREAKDOWN_ENTRY_STATE 6

uint8_t stateBeforeEm = 0;
uint8_t Emergency = FALSE;

uint8_t stateRegular = 66;
uint8_t stateEmergency = 0;
int elapsedMillis = 0;      // Count elapsed milliseconds from last state change

ISR(INT0_vect) {
    Emergency = TRUE;
    stateEmergency = 0;
    stateBeforeEm = stateRegular;
}

ISR(TIMER1_COMPA_vect) {
    elapsedMillis += 10;
}

void tc1_init(){
    TCCR1B = 0;                         // STOP the timer
    TIFR1 = (7<<TOV1) | (1<<ICF1);      // Clear flag register
    TCCR1A = 0;                         // Set mode...
    TCCR1B = (1<<WGM12);                // ...to CTC
    OCR1A = 625;                        // OCR1A
    TIMSK1 = (1<<OCIE1A);               // Enable interrupt on compare with OCR1A
    TCCR1B |= 4;                        // Set the TP to 256
}

int main() {

    // Semaphores
    DDRB |= (1 << RNS); // NS red
    DDRB |= (1 << YNS); // NS yellow
    DDRB |= (1 << GNS); // NS green
    DDRB |= (1 << REW); // EW red
    DDRB |= (1 << YEW); // EW yellow
    DDRB |= (1 << GEW); // EW green

    DDRD &= ~(1 << EMG_BUTTON); // Set as Input
    PORTD |= (1 << EMG_BUTTON); // Activate the internal Pull-up

    // EMG button interrupt setup
    EICRA |= (0x02);        // INT0 Interrupt at FE
    EIMSK |= (1 << INT0);

    // INIT Timers
    tc1_init();

    sei();
    
    
    while (1) {
        
        if (TRUE == Emergency) {
            // STATE MACHINE - EMERGENCY
            switch(stateEmergency) {
                case 0:
                    if (1 == stateBeforeEm || 2 == stateBeforeEm) {
                        stateEmergency = 1;
                        stateRegular = 3;
                    }
                    else if (4 == stateBeforeEm || 5 == stateBeforeEm) {
                        stateEmergency = 2;
                        stateRegular = 0;
                    }
                    else {
                        stateEmergency = 3;
                    }
                    elapsedMillis = 0;
                    break;
                case 1:
                    if (YELLOW_DELAY <= elapsedMillis) {
                        stateEmergency = 3;
                        elapsedMillis = 0;
                    }
                    break;
                case 2:
                    if (YELLOW_DELAY <= elapsedMillis) {
                        stateEmergency = 3;
                        elapsedMillis = 0;
                    }
                    break;
                case 3:
                    if (STOPPED_DELAY <= elapsedMillis) {
                        Emergency = FALSE;
                        elapsedMillis = 0;
                    }
                    break;
                default:
                    Emergency = FALSE;
                    stateRegular = BREAKDOWN_ENTRY_STATE;
                    break;
            }
        }
        else {
            // STATE MACHINE - REGULAR
            switch(stateRegular) {
                case 0:
                    if (RED_DELAY <= elapsedMillis) {
                        stateRegular = 1;
                        elapsedMillis = 0; // Reset counter
                    }
                    break;
                case 1:
                    if (GREEN_DELAY <= elapsedMillis) {
                        stateRegular = 2;
                        elapsedMillis = 0; // Reset counter
                    }
                    break;
                case 2:
                    if (YELLOW_DELAY <= elapsedMillis) {
                        stateRegular = 3;
                        elapsedMillis = 0; // Reset counter
                    }
                    break;
                case 3:
                    if (RED_DELAY <= elapsedMillis) {
                        stateRegular = 4;
                        elapsedMillis = 0; // Reset counter
                    }
                    break;
                case 4:
                    if (GREEN_DELAY <= elapsedMillis) {
                        stateRegular = 5;
                        elapsedMillis = 0; // Reset counter
                    }
                    break;
                case 5:
                    if (YELLOW_DELAY <= elapsedMillis) {
                        stateRegular = 0;
                        elapsedMillis = 0; // Reset counter
                    }
                    break;
                case 6: // BREAKDOWN STATE
                    if (BREAKDOWN_DELAY <= elapsedMillis) {
                        stateRegular = 7;
                        elapsedMillis = 0; // Reset counter
                    }
                    break;
                case 7: // BREAKDOWN STATE
                    if (BREAKDOWN_DELAY <= elapsedMillis) {
                        stateRegular = 6;
                        elapsedMillis = 0; // Reset counter
                    }
                    break;
                default:
                    stateRegular = BREAKDOWN_ENTRY_STATE;
                    break;
            }
        }


        // Change Output Pin Values
        if (TRUE == Emergency) {
            switch(stateEmergency) {
                case 0:
                    break;
                case 1:
                    PORTB &= ~(1 << RNS);
                    PORTB |= (1 << YNS);
                    PORTB &= ~(1 << GNS);
                    PORTB |= (1 << REW);
                    PORTB &= ~(1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                case 2:
                    PORTB |= (1 << RNS);
                    PORTB &= ~(1 << YNS);
                    PORTB &= ~(1 << GNS);
                    PORTB &= ~(1 << REW);
                    PORTB |= (1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                case 3:
                    PORTB |= (1 << RNS);
                    PORTB &= ~(1 << YNS);
                    PORTB &= ~(1 << GNS);
                    PORTB |= (1 << REW);
                    PORTB &= ~(1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                default:
                    Emergency = FALSE;
                    stateRegular = BREAKDOWN_ENTRY_STATE;
                    break;
            }
        }
        else {
            switch (stateRegular) {
                case 0:
                    PORTB |= (1 << RNS);  // RNS ON
                    PORTB &= ~(1 << YNS);
                    PORTB &= ~(1 << GNS); // GNS off
                    PORTB |= (1 << REW);
                    PORTB &= ~(1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                case 1:
                    PORTB &= ~(1 << RNS);
                    PORTB &= ~(1 << YNS);
                    PORTB |= (1 << GNS);
                    PORTB |= (1 << REW);
                    PORTB &= ~(1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                case 2:
                    PORTB &= ~(1 << RNS);
                    PORTB |= (1 << YNS);
                    PORTB &= ~(1 << GEW);
                    PORTB |= (1 << REW);
                    PORTB &= ~(1 << YEW);
                    PORTB &= ~(1 << GNS);
                    break;
                case 3:
                    PORTB |= (1 << RNS);
                    PORTB &= ~(1 << YNS);
                    PORTB &= ~(1 << GNS);
                    PORTB |= (1 << REW);
                    PORTB &= ~(1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                case 4:
                    PORTB |= (1 << RNS);
                    PORTB &= ~(1 << YNS);
                    PORTB &= ~(1 << GNS);
                    PORTB &= ~(1 << REW);
                    PORTB &= ~(1 << YEW);
                    PORTB |= (1 << GEW);
                    break;
                case 5:
                    PORTB |= (1 << RNS);
                    PORTB &= ~(1 << YNS);
                    PORTB &= ~(1 << GNS);
                    PORTB &= ~(1 << REW);
                    PORTB |= (1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                case 6:
                    PORTB &= ~(1 << RNS);
                    PORTB |= (1 << YNS);
                    PORTB &= ~(1 << GNS);
                    PORTB &= ~(1 << REW);
                    PORTB &= ~(1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                case 7:
                    PORTB &= ~(1 << RNS);
                    PORTB &= ~(1 << YNS);
                    PORTB &= ~(1 << GNS);
                    PORTB &= ~(1 << REW);
                    PORTB |= (1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                default:
                    stateRegular = BREAKDOWN_ENTRY_STATE;
                    break;
            }
        }
    }
}