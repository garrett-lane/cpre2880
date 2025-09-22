#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "sensor-data.h"

#define PI 3.14159
#define ALPHA 0.1

typedef struct detect_object {
    float center_angle;
    float distance;
    int linear_width;
} Object_t;

Object_t create_object(float start_dist, float end_dist, int start_angle, int end_angle) {
    struct detect_object object;

    object.center_angle = (start_angle + end_angle) / 2.0;
    object.distance = (start_dist + end_dist) / 2.0;
    object.linear_width = object.distance * ((end_angle - start_angle) * (PI / 180.0));

    return object;
}

float evaluate_data(float data_array[], int arrLength) {
    float start_dist = 0; // current object distance
    float end_dist = 0;
    int start_deg = 0; // current object angle
    int end_deg = 0;

    int in_object = 0;

    // Object_t smallest_object;
    float smallest_obj_ctr_angle = 999.0;
    float smallest_obj_lin_width = 999.0;
    Object_t current_object;

    int i = 1;
    for (i = 1; i < arrLength; i++) {
        // if distance is < 8% of prev. value, start object
        if (!in_object && data_array[i] < 0.85 * data_array[i-1]) {
            start_deg = i * 2; // starting degree
            start_dist = data_array[i]; // initial angle
            in_object = 1;
        } else if (in_object && data_array[i] > 1.15 * data_array[i-1]) { // if distance is > 115% of prev. data item, end object
            end_deg = (i-1) * 2;
            end_dist = data_array[i-1];

            in_object = 0;

            current_object = create_object(start_dist, end_dist, start_deg, end_deg);

            if (current_object.linear_width < 6) {
                continue;
            } else {
                printf("OBJECT %f %f %d\n", current_object.center_angle, current_object.distance, current_object.linear_width);
            }

            if (current_object.linear_width < smallest_obj_lin_width) {
                smallest_obj_lin_width = current_object.linear_width;
                smallest_obj_ctr_angle = current_object.center_angle;
            }
        }
    }

    return smallest_obj_ctr_angle;
}

void expontential_smoothing(int index, float *cleaned_data) {
    if (index == 0) {
        cleaned_data[index] = sensor_data_array[index];
    } else if (index == 1 || index == 2) {
        cleaned_data[index] = ALPHA * sensor_data_array[index] + (1 - ALPHA) * cleaned_data[index - 1];
    } else {
        float avg = (sensor_data_array[index] + sensor_data_array[index - 1] + sensor_data_array[index - 2]) / 3.0;
        if (avg > 0.90 * sensor_data_array[index - 1] && avg  < 1.1 * sensor_data_array[index - 1]) {
            cleaned_data[index] = ALPHA * sensor_data_array[index] + (1 - ALPHA) * cleaned_data[index - 1];
        } else {
            cleaned_data[index] = sensor_data_array[index];
        }
    }
}

int main (void) {
    int arrLength = sizeof(sensor_data_array) / sizeof(sensor_data_array[0]);
    float cleaned_data[arrLength + 1];

    printf("STARTSCAN\n");
    for (int i = 0; i < arrLength; i++) {
        printf("%3d\t\t\t\t%4f\n", i * 2, sensor_data_array[i]);
        expontential_smoothing(i, cleaned_data);
    }
    printf("ENDSCAN\n");

    printf("STARTSCAN\n");
    for (int i = 0; i < arrLength; i++) {
        printf("%3d\t\t\t\t%4f\n", i * 2, cleaned_data[i]);
    }
    printf("ENDSCAN\n");

    evaluate_data(cleaned_data, arrLength + 1);
}

