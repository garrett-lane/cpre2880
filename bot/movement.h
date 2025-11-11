/*
 * movement.h
 *
 *  Created on: Sep 7, 2025
 *      Author: gthomp
 */

#ifndef MOVEMENT_H_
#define MOVEMENT_H_
#include "open_interface.h"

// Drive FORWARD in a straight line for distance_mm MILLIMETERS at speed_mm MM/S. All values should always be POSITIVE.
int move_forward(oi_t *sensor, int distance_mm, int speed_mm);

// Drive BACKWARD in a straight line for distance_mm MILLIMETERS at speed_mm MM/S. All values should always be POSITIVE.
int move_backward(oi_t *sensor, int distance_mm, int speed_mm);

// Turn CLOCKWISE in for degrees DEGREES at speed_mm MM/S, halves the speed_MM for the last 30% of the turn. All values should always be POSITIVE.
int turn_clockwise(oi_t *sensor, int degrees, int speed_mm);

// Turn COUNTERCLOCKWISE in for degrees DEGREES at speed_mm MM/S, halves the speed_MM for the last 30% of the turn. All values should always be POSITIVE.
int turn_counterclockwise(oi_t *sensor, int degrees, int speed_mm);

#endif /* MOVEMENT_H_ */
