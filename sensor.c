#include <math.h>
#include <stdlib.h>

#include "timer.h"
#include "lcd.h"

#include "adc.h"
#include "putty.h"
#include "sensor.h"
#include "calibration.h"
#include "servo.h"
#include "ping.h"

#define PI 3.14159
// Smoothing factor for ping sensor
#define SCAN_ALPHA_SOUND 0.1
// Smoothing factor for IR sensor
#define SCAN_ALPHA_IR 0.25

int scan_index = 0;
float center_distance; // CM

// Performs a servo scan across specified angle range, calls callback on each angle scanned
void scan_range(int lower_angle, int upper_angle, int step, cyBOT_Scan_t *scan_data, void (*callback_ptr)(int angle, cyBOT_Scan_t *scan_data), int num_scans) {
    scan_index = 0;
    int i, j;
    cyBOT_Scan_t *avg_scan = (cyBOT_Scan_t *) calloc(1, sizeof(cyBOT_Scan_t));
    for (i = lower_angle; i <= upper_angle; i += step) {
        memset(avg_scan, 0, sizeof(cyBOT_Scan_t));

        servo_move(i);
        for (j = 0; j < num_scans; j++) {
            adc_read();

            while (!adc_flag) {}

            avg_scan->IR_raw_val += adc_data;
            avg_scan->sound_dist += ping_read();

            adc_flag = 0;
        }

        avg_scan->IR_raw_val /= num_scans;
        avg_scan->sound_dist /= num_scans;
        callback_ptr(i, avg_scan);
    }

    free(avg_scan);
}

// Process noisy data using exponential smoothing
void process_noisy_data(int angle, cyBOT_Scan_t *scan_data) {
    // Voltage to cm
    double vToCm = VTOCM_COEFF * pow(scan_data->IR_raw_val, VTOCM_EXP);

    // First scan value
    if (scan_index == 0) {
        scan_sensor_data_array[scan_index].sound_dist = scan_data->sound_dist;
        scan_sensor_data_array[scan_index].IR_raw_val = vToCm < 100 ? vToCm : 100;
        goto process_noisy_data_exit;
    }

    cyBOT_Scan_t obj;

    // Smooth ping
    if (scan_data->sound_dist > 0.85 * scan_sensor_data_array[scan_index - 1].sound_dist &&
            scan_data->sound_dist < 1.15 * scan_sensor_data_array[scan_index - 1].sound_dist) {
        obj.sound_dist = SCAN_ALPHA_SOUND * scan_data->sound_dist + (1 - SCAN_ALPHA_SOUND) * scan_sensor_data_array[scan_index - 1].sound_dist;
    } else {
        obj.sound_dist = scan_data->sound_dist;
    }

    // Smooth IR
    if (vToCm > 0.7 * scan_sensor_data_array[scan_index - 1].IR_raw_val &&
            vToCm < 1.30 * scan_sensor_data_array[scan_index - 1].IR_raw_val) {
        obj.IR_raw_val = SCAN_ALPHA_IR * vToCm + (1 - SCAN_ALPHA_IR) * scan_sensor_data_array[scan_index - 1].IR_raw_val;
    } else {
        // Check for spike
        if (scan_index > 1 && (vToCm > 0.9 * scan_sensor_data_array[scan_index - 2].IR_raw_val &&
                vToCm < 1.10 * scan_sensor_data_array[scan_index - 2].IR_raw_val)) {
            obj.IR_raw_val = SCAN_ALPHA_IR * vToCm + (1 - SCAN_ALPHA_IR) * scan_sensor_data_array[scan_index - 2].IR_raw_val;

            // Replace old spike
            scan_sensor_data_array[scan_index - 1].IR_raw_val = obj.IR_raw_val < 100 ? obj.IR_raw_val : 100;
            putty_printf("%3d\t%10f\t%10d\n", angle - 2, scan_sensor_data_array[scan_index - 1].sound_dist, scan_sensor_data_array[scan_index - 1].IR_raw_val);
            timer_waitMillis(100);

        } else {
            obj.IR_raw_val = vToCm;
        }
    }

    // Max distance of 100 cm because smoothing goes off to infinity
    if (obj.IR_raw_val > 100) {
        obj.IR_raw_val = 100;
    }

    scan_sensor_data_array[scan_index] = obj;

process_noisy_data_exit:
    // Send scan data to GUI
    putty_send_scan_data(angle, &scan_sensor_data_array[scan_index]);
    lcd_clear();
    lcd_printf("%3d %5f %5d\n", angle, scan_sensor_data_array[scan_index].sound_dist, scan_sensor_data_array[scan_index].IR_raw_val);
    scan_index++;
}

// Callback to get center distance of object
void get_center_distance(int angle, cyBOT_Scan_t *scan_data) {
    lcd_printf("%3d %5f %5d\n", angle, scan_data->sound_dist, scan_data->IR_raw_val);

    center_distance = scan_data->sound_dist;
}

// Performs a scan between two angles with 2-degree increments
void scan_full_arc(cyBOT_Scan_t *scan_data, int start_angle, int stop_angle) {
    // Move to start angle
    servo_move(start_angle);
    timer_waitMillis(500);

    // Tell GUI we're doing a scan
    putty_printf("STARTSCAN %d 2\n", ((stop_angle - start_angle) / 2) + 1);
    timer_waitMillis(100);
    scan_range(start_angle, stop_angle, 2, scan_data, process_noisy_data, 3);
    timer_waitMillis(200);
    putty_printf("ENDSCAN\n");
}

// Creates an object struct
Object_t create_object(float dist, int start_angle, int end_angle) {
    struct detect_object object;

    object.center_angle = (start_angle + end_angle) / 2.0;
    object.distance = dist;
    object.linear_width = object.distance * ((end_angle - start_angle) * (PI / 180.0));

    return object;
}

// Updates the linear width of an object
void object_update_linear_width(Object_t *obj, int start_angle, int end_angle) {
    obj->linear_width = obj->distance * ((end_angle - start_angle) * (PI / 180.0));
}

// Finds and returns the smallest object detected in scan range
Object_t find_smallest_objects(cyBOT_Scan_t data_array[], int start_angle, int end_angle, cyBOT_Scan_t *scan_data) {
    float dist = 999.9;
    int start_deg = 0; // current object angle
    int end_deg = 0;
    int objStartIndex = 0;

    int in_object = 0;

    Object_t current_object;

    Object_t smallest_object;
    smallest_object.center_angle = 0.0;
    smallest_object.distance = 0.0;
    smallest_object.linear_width = 999.0;

    int i = 1;
    for (i = 1; i < ((end_angle - start_angle) / 2) + 1; i++) {
        // if distance is < 8% of prev. value, start object
        if (!in_object && data_array[i].IR_raw_val < 150 && data_array[i].IR_raw_val < 0.85 * data_array[i-1].IR_raw_val) {
            start_deg = start_angle + (i * 2); // starting degree
            in_object = 1;
            objStartIndex = i;
        } else if (in_object && data_array[i].IR_raw_val > 1.15 * data_array[i-1].IR_raw_val) { // if distance is > 115% of prev. data item, end object
            end_deg = start_angle + ((i-1) * 2);
            dist = data_array[(i-objStartIndex) / 2].IR_raw_val;

            in_object = 0;

            current_object = create_object(dist, start_deg, end_deg);
            current_object.center_angle = (end_deg + start_deg) / 2;
            servo_move(current_object.center_angle);
            timer_waitMillis(1000);
            scan_range(current_object.center_angle, current_object.center_angle, 1, scan_data, get_center_distance, 5);
            current_object.distance = center_distance;
            object_update_linear_width(&current_object, start_deg, end_deg);

            if (current_object.linear_width > 0 && current_object.distance < 150) {
                putty_send_object(&current_object);
            }

            if (current_object.linear_width < smallest_object.linear_width && current_object.distance < 150 && current_object.linear_width > 0) {
                smallest_object = current_object;
            }
        }
    }

    return smallest_object;
}
