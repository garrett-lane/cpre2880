#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "sensor-data.h"

#define ALPHA 0.1

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
}

