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
    int start;
    int end;
} Object_t;

cyBOT_Scan_t scan_sensor_data_array[91];

// Uses the servo and distance sensors to scan an arc from LOWER_ANGLE to UPPER_ANGLE stepping STEP each iter.
// CALLBACK_PTR(int, cyBOT_Scan_t) is called on each new scan step
// cyBOT_init_Scan *must* have been previously called with the servo enabled
void scan_range(int lower_angle, int upper_angle, int step, cyBOT_Scan_t *scan_data, void (*callback_ptr)(int angle, cyBOT_Scan_t *scan_data), int num_scans);
void scan_full_arc(cyBOT_Scan_t *scan_data, int start_angle, int stop_angle);

// Process noisy data using exponential smoothing
void process_noisy_data(int angle, cyBOT_Scan_t *scan_data);

// Callback to get center distance of object
void get_center_distance(int angle, cyBOT_Scan_t *scan_data);

// Creates an object struct
Object_t create_object(float dist, int start_angle, int end_angle);

// Updates the linear width of an object
void object_update_linear_width(Object_t *obj, int start_angle, int end_angle);

// Finds and returns the smallest object detected in scan range
Object_t find_smallest_objects(cyBOT_Scan_t data_array[], int start_angle, int end_angle, cyBOT_Scan_t *scan_data);

#endif /* SENSOR_H_ */
