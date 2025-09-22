/*
 * helper.c
 *
 *  Created on: Sep 19, 2025
 *      Author: seankru
 */

#include "sensor.h"
#include "timer.h"

#include "putty.h"

#define PI 3.14159
#define SCAN_ALPHA 0.1

int scan_index = 0;

void scan_range(int lower_angle, int upper_angle, int step, cyBOT_Scan_t *scan_data, void (*callback_ptr)(int angle, cyBOT_Scan_t *scan_data)) {
    scan_index = 0;
    int i;
    for (i = lower_angle; i <= upper_angle; i += step) {
        cyBOT_Scan(i, scan_data);
        callback_ptr(i, scan_data);
    }
}

void process_noisy_data(int angle, cyBOT_Scan_t *scan_data) {
    if (scan_index == 0) {
        scan_sensor_data_array[scan_index] = scan_data->sound_dist;
        goto process_noisy_data_exit;
    }

    if (scan_data->sound_dist > 0.85 * scan_sensor_data_array[scan_index - 1] &&
            scan_data->sound_dist < 1.15 * scan_sensor_data_array[scan_index - 1]) {
        scan_sensor_data_array[scan_index] = SCAN_ALPHA * scan_data->sound_dist + (1 - SCAN_ALPHA) * scan_sensor_data_array[scan_index - 1];
    } else {
        scan_sensor_data_array[scan_index] = scan_data->sound_dist;
    }

process_noisy_data_exit:
#if DEBUG
    putty_send_scan_data(angle, scan_data);
#else
    putty_printf("%3d\t\t%10f\n", angle, scan_sensor_data_array[scan_index]);
#endif
    scan_index++;
}

Object_t create_object(float start_dist, float end_dist, int start_angle, int end_angle) {
    struct detect_object object;

    object.center_angle = (start_angle + end_angle) / 2.0;
    object.distance = (start_dist + end_dist) / 2.0;
    object.linear_width = object.distance * ((end_angle - start_angle) * (PI / 180.0));

    return object;
}
