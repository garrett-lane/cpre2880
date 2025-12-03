/*
 * calibration.h
 *
 *  Created on: Sep 20, 2025
 *      Author: seankru
 */

#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include "sensor.h"
#include "button.h"

#include "open_interface.h"

#define CALIBRATION_180_DEG 229
#define CALIBRATION_0_DEG -100

#define STRAIGHT_MULTIPLIER 0.995 // typically overshoots by 0.05% with a multiplier of 1
#define TURN_MULTIPLIER 1 // don't think should ever need to change

// requires regular calibration per robot
#define VTOCM_COEFF 195624.387
#define VTOCM_EXP -1.27217

void calibrate_ir(oi_t *data);
void calibrate_servo();
double avg_ir_scan();

#endif /* CALIBRATION_H_ */
