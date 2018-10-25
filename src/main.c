/*
 * main.c
 *
 *  Created on: Oct 10, 2018
 *      Author: Luís Sousa, Leonor Santos
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <timer_tools.h>

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

uint8_t stateRegular = 0;
uint8_t stateEmergency = 0;
int elapsedMillis = 0;      // Count elapsed milliseconds from last state change

ISR(INT0_vect) {
    Emergency = TRUE;
    stateEmergency = 0;
    stateBeforeEm = stateRegular;
}

int main() {
    // Set up timer
    TCCR1B |= (1 << CS10);

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
    EICRA |= (0x02);   // INT0 Interrupt at FE
    EIMSK |= (1 << INT0);

    sei();
    
    mili_timer T1;
    init_mili_timers();
    start_timer(&T1, 500);   // Start a timer to count 1000 miliseconds

    while (1) {

        if (get_timer(&T1)) {
            start_timer(&T1, 500); // Restart the timer
            //_delay_ms(10);
            elapsedMillis += 500;
        }
        
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
                    stateEmergency = BREAKDOWN_ENTRY_STATE;
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