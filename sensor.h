/*
 * helper.h
 *
 *  Created on: Sep 19, 2025
 *      Author: seankru
 *      Sub-author: gthomp
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include "cyBot_Scan.h"

typedef struct detect_object {
    float center_angle;
    float distance;
    int linear_width;
} Object_t;

float scan_sensor_data_array[91];

// Uses the servo and distance sensors to scan an arc from LOWER_ANGLE to UPPER_ANGLE stepping STEP each iter.
// CALLBACK_PTR(int, cyBOT_Scan_t) is called on each new scan step
// cyBOT_init_Scan *must* have been previously called with the servo enabled
void scan_range(int lower_angle, int upper_angle, int step, cyBOT_Scan_t *scan_data, void (*callback_ptr)(int angle, cyBOT_Scan_t *scan_data));

// Process noisy data using exponential smoothing
void process_noisy_data(int angle, cyBOT_Scan_t *scan_data);

// An example of the callback_ptr that sends the current scan data over UART
void send_data(int angle, cyBOT_Scan_t *scan_data);

Object_t create_object(float start_dist, float end_dist, int start_angle, int end_angle);

#endif /* SENSOR_H_ */
