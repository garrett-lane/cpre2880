/*
 * balloon.h
 *
 *  Created on: Dec 2nd, 2025
 *      Author: gthomp
 */

#ifndef BALLOON_H_
#define BALLOON_H_

#include <inc/tm4c123gh6pm.h>
#include <stdbool.h>
#include <stdint.h>

#include "driverlib/interrupt.h"

volatile int balloon_flag;

// Initialize balloon sensor on GPIO Port D pin 0
// Configures falling-edge interrupt to detect balloon contact
void balloon_init();

// Reads current state of balloon sensor
int read_balloon();

// Interrupt handler for balloon detection
// Sets flag when balloon contact is detected
void balloon_handler();

#endif /* BALLOON_H_ */
