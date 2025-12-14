/*
 * calibration.h
 *
 *  Created on: Sep 20, 2025
 *      Author: seankru
 */

#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include "button.h"
#include "open_interface.h"
#include "sensor.h"

#define CALIBRATION_0_DEG -102   // BOT 1
#define CALIBRATION_180_DEG 214  // BOT 1
// #define CALIBRATION_0_DEG -106 // BOT 2
// #define CALIBRATION_180_DEG 217 // BOT 2

#define STRAIGHT_MULTIPLIER \
    0.995  // typically overshoots by 0.05% with a multiplier of 1
#define TURN_MULTIPLIER 1  // don't think should ever need to change

// requires regular calibration per robot
#define VTOCM_COEFF 158347.537506  // BOT 1
#define VTOCM_EXP -1.212053        // BOT 1
// #define VTOCM_COEFF 58172.587537 // BOT 2
// #define VTOCM_EXP -1.068098 // BOT 2

// Calibrates IR sensor by comparing readings with ultrasonic sensor
void calibrate_ir(oi_t *data);

// Servo calibration using buttons
void calibrate_servo();

// Averages multiple IR sensor readings
double avg_ir_scan();

#endif /* CALIBRATION_H_ */
