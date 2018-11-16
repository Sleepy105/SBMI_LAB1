/*
 *  main.c
 *
 *  Created on: Oct 10, 2018
 *      Author: Luís Sousa (lm.sousa@fe.up.pt), Leonor Santos (up201504515@fe.up.pt)
 * 
 *  This code is designed to control a simple semaphore.
 *  It implements 6 different lights. 3 for each traffic direction.
 *  Using external interrupts, it allows for an emergency stop of
 *  all traffic flow.
 *  In addition, this code is designed to set the system in a
 *  self-protection state in case of memory degradation/error.
 *  In this state the system is considered to be compromised and
 *  will ignore all external inputs and wait for a system-wide reset.
 *  
 *  By using timer-triggered interruptions to count the time there
 *  are no points where the system 'hangs'.
 * 
 *  Technical Information:
 *      Timer used for time-keeping: Timer1
 *      System CP: 1 (Arduino Default)
 *      Timer1 TP: 256
 *      External Interrupts used: INT0
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

#define RED_DELAY 5000
#define YELLOW_DELAY 5000
#define GREEN_DELAY 50000
#define BREAKDOWN_DELAY 1000
#define STOPPED_DELAY 10000

#define BREAKDOWN_ENTRY_STATE 6
#define EMERGENCY_ENTRY_STATE 8

volatile uint8_t stateBeforeEm = 0;
uint8_t Emergency = FALSE;

volatile uint8_t stateRegular = 0;
volatile int remainingMillis = 0;       // Count elapsed milliseconds from last state change

ISR(INT0_vect) {                        // Enter 'Emergency' mode on button press
    if (!Emergency) {
        stateBeforeEm = stateRegular;   // Save the system state before entering emergency mode
    }
    stateRegular = EMERGENCY_ENTRY_STATE;
}

ISR(TIMER1_COMPA_vect) {
    if (remainingMillis) {
        remainingMillis -= 10;
    }
}

void tc1_init() {                       // Initialize Timer1 to trigger a interrupt every 10ms
    TCCR1B = 0;                         // STOP the timer
    TIFR1 = (7<<TOV1) | (1<<ICF1);      // Clear flag register
    TCCR1A = 0;                         // Set mode...
    TCCR1B = (1<<WGM12);                // ...to CTC
    OCR1A = 625;                        // OCR1A
    TIMSK1 = (1<<OCIE1A);               // Enable interrupt on compare with OCR1A
    TCCR1B |= 4;                        // Set the TP to 256, therefore also starting the timer
}

void hardware_Init() {
    /* Stoplight LEDs as outputs */
    DDRB |= (1 << RNS);                 // NS red
    DDRB |= (1 << YNS);                 // NS yellow
    DDRB |= (1 << GNS);                 // NS green
    DDRB |= (1 << REW);                 // EW red
    DDRB |= (1 << YEW);                 // EW yellow
    DDRB |= (1 << GEW);                 // EW green

    /* EMG_BUTTON as input */
    DDRD &= ~(1 << EMG_BUTTON);         // Set as Input
    PORTD |= (1 << EMG_BUTTON);         // Activate the internal Pull-up

    /* EMG button interrupt setup */
    EICRA |= (0x02);                    // INT0 Interrupt at FE
    EIMSK |= (1 << INT0);               // Enable the INT0 external interrupt call

    /* Activate Interrupts */
    sei();
}

void disableExternalInputs() {
    EIMSK &= ~(1 << INT0);              // Disable the INT0 external interrupt call
}

int main() {

    /* Hardware Initialization */
    hardware_Init();    
    tc1_init();


    remainingMillis = RED_DELAY;
  
    while (1) {
        
        switch(stateRegular) {
            case 0:
                if (0 == remainingMillis) {
                    stateRegular = 1;
                    remainingMillis = GREEN_DELAY;      // Reset counter
                }

                PORTB = (1 << RNS) | (1 << REW);
                break;
            case 1:
                if (0 == remainingMillis) {
                    stateRegular = 2;
                    remainingMillis = YELLOW_DELAY;     // Reset counter
                }

                PORTB = (1 << GNS) | (1 << REW);
                break;
            case 2:
                if (0 == remainingMillis) {
                    stateRegular = 3;
                    remainingMillis = RED_DELAY;        // Reset counter
                }

                PORTB = (1 << YNS) | (1 << REW);
                break;
            case 3:
                if (0 == remainingMillis) {
                    stateRegular = 4;
                    remainingMillis = GREEN_DELAY;      // Reset counter
                }

                PORTB = (1 << RNS) | (1 << REW);
                break;
            case 4:
                if (0 == remainingMillis) {
                    stateRegular = 5;
                    remainingMillis = YELLOW_DELAY;     // Reset counter
                }

                PORTB = (1 << RNS) | (1 << GEW);
                break;
            case 5:
                if (0 == remainingMillis) {
                    stateRegular = 0;
                    remainingMillis = RED_DELAY;        // Reset counter
                }

                PORTB = (1 << RNS) | (1 << YEW);
                break;
            


            /* BREAKDOWN STATES */
            case 6:
                if (0 == remainingMillis) {
                    stateRegular = 7;
                    remainingMillis = BREAKDOWN_DELAY;  // Reset counter
                }

                PORTB = (1 << YNS);
                break;
            case 7:
                if (0 == remainingMillis) {
                    stateRegular = 6;
                    remainingMillis = BREAKDOWN_DELAY;  // Reset counter
                }

                PORTB = (1 << YEW);
                break;
            


            /* EMERGENCY STATES */
            case 8:
                if (!Emergency) {
                    switch(stateBeforeEm) {
                        /*
                         * Only reset the timer if the state is one where a Green light was on.
                         * Otherwise just keep the Yellow light on for the remaining time it is
                         * supposed to.
                         */
                        
                        case 1:
                            remainingMillis = YELLOW_DELAY;
                        case 2:
                            stateRegular = 9;
                            break;
                        case 4:
                            remainingMillis = YELLOW_DELAY;
                        case 5:
                            stateRegular = 10;
                            break;
                        default:
                            stateRegular = 11;
                            remainingMillis = STOPPED_DELAY;
                            break;
                    }
                }
                else {
                    stateRegular = 11;
                    remainingMillis = STOPPED_DELAY;
                }

                Emergency = TRUE;
                break;
            case 9:
                if (0 == remainingMillis) {
                    stateRegular = 11;
                    remainingMillis = STOPPED_DELAY;
                }

                PORTB = (1 << YNS) | (1 << REW);
                break;
            case 10:
                if (0 == remainingMillis) {
                    stateRegular = 11;
                    remainingMillis = STOPPED_DELAY;
                }

                PORTB = (1 << RNS) | (1 << YEW);
                break;
            case 11:
                if (0 == remainingMillis) {
                    if (1 == stateBeforeEm || 2 == stateBeforeEm) {
                        stateRegular = 4;
                        remainingMillis = GREEN_DELAY;
                    }
                    else if (4 == stateBeforeEm || 5 == stateBeforeEm) {
                        stateRegular = 1;
                        remainingMillis = GREEN_DELAY;
                    }
                    else {
                        stateRegular = stateBeforeEm;
                    }
                    Emergency = FALSE;
                }

                PORTB = (1 << RNS) | (1 << REW);
                break;
            default:                                    // In case of system degradation:
                stateRegular = BREAKDOWN_ENTRY_STATE;
                remainingMillis = BREAKDOWN_DELAY;
                disableExternalInputs();                // System is considered compromised. All external inputs disabled. Wait for system-wide reset
                Emergency = FALSE;                      // 'Breakdown' state is not a 'Emergency' state
                break;
        }
    }
}