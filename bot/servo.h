/*
 * servo.h
 *
 *  Created on: Nov 4, 2025
 *      Author: gthomp
 */

#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>
#include <inc/tm4c123gh6pm.h>
#include <stdbool.h>
#include "driverlib/interrupt.h"

int current_degrees;

void servo_init(void);
void servo_move(float degrees);

void set_servo_calibation(float value_z, float value_one_eighty);

#endif /* SERVO_H_ */
