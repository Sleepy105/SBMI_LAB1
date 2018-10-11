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

#define RED_DELAY 5000
#define YELLOW_DELAY 5000
#define GREEN_DELAY 50000
#define BREAKDOWN_DELAY 1000

uint8_t Breakdown = FALSE;
uint8_t stateRegular = 0;
uint8_t stateBreakdown = 0;
int elapsedMillis = 0;


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
    
    mili_timer T1;
    init_mili_timers();
    start_timer(&T1, 1000);   // Start a timer to count 1000 miliseconds

    while (1) {

        if (get_timer(&T1)) {
            start_timer(&T1, 1000); // Restart the timer
            elapsedMillis += 1000;
        }
        

        if (Breakdown == TRUE) {
            // STATE MACHINE - BREAKDOWN
            if (stateBreakdown == 0 && elapsedMillis >= BREAKDOWN_DELAY) {
                stateBreakdown = 1;
                elapsedMillis = 0;
            } else if (stateBreakdown == 1 && elapsedMillis >= BREAKDOWN_DELAY) {
                stateBreakdown = 0;
                elapsedMillis = 0;
            }
        }
        else {
            // STATE MACHINE - REGULAR
            if (stateRegular == 0 && elapsedMillis >= RED_DELAY) {
                stateRegular = 1;
                elapsedMillis = 0; // Reset counter
            } else if (stateRegular == 1 && elapsedMillis >= GREEN_DELAY) {
                stateRegular = 2;
                elapsedMillis = 0;
            } else if (stateRegular == 2 && elapsedMillis >= YELLOW_DELAY) {
                stateRegular = 3;
                elapsedMillis = 0;
            } else if (stateRegular == 3 && elapsedMillis >= RED_DELAY) {
                stateRegular = 4;
                elapsedMillis = 0;
            } else if (stateRegular == 4 && elapsedMillis >= GREEN_DELAY) {
                stateRegular = 5;
                elapsedMillis = 0;
            } else if (stateRegular == 5 && elapsedMillis >= YELLOW_DELAY) {
                stateRegular = 0;
                elapsedMillis = 0;
            }
        }


        // Change Output Pin Values
        if (Breakdown == FALSE) {
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
                default:
                    stateRegular = 0;
                    break;
            }
        }
        else {
            switch(stateBreakdown) {
                case 0:
                    PORTB &= ~(1 << RNS);  // RNS ON
                    PORTB |= (1 << YNS);
                    PORTB &= ~(1 << GNS); // GNS off
                    PORTB &= ~(1 << REW);
                    PORTB |= (1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                case 1:
                    PORTB &= ~(1 << RNS);  // RNS ON
                    PORTB &= ~(1 << YNS);
                    PORTB &= ~(1 << GNS); // GNS off
                    PORTB &= ~(1 << REW);
                    PORTB &= ~(1 << YEW);
                    PORTB &= ~(1 << GEW);
                    break;
                default:
                    stateBreakdown = 0;
                    break;
            }
        }
    }
}