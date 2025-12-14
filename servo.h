/*
 * servo.h
 *
 *  Created on: Nov 4, 2025
 *      Author: gthomp
 */

#ifndef SERVO_H_
#define SERVO_H_

#include <inc/tm4c123gh6pm.h>
#include <stdbool.h>
#include <stdint.h>

#include "driverlib/interrupt.h"

int current_degrees;

// Initializes the servo connected to Port B Pin 5
void servo_init(void);

// Sets the PWM rate to point the servo in a specific direction
void servo_move(float degrees);

// Use pre-defined values to calculate proper offsets for the servo
void set_servo_calibation(float value_z, float value_one_eighty);

#endif /* SERVO_H_ */
