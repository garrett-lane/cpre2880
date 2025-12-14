// Sensor calibration functions for CyBot
// Calibrates IR sensor using power regression and servo alignment

#include "calibration.h"

#include <math.h>

#include "adc.h"
#include "lcd.h"
#include "movement.h"
#include "ping.h"
#include "putty.h"
#include "sensor.h"
#include "servo.h"
#include "timer.h"

#define CAL_IR_NUM_ATTEMPTS 5.0

double ping_distances[10];
double ir_distances[10];

double avg_ping_scan();
double avg_ir_scan();
void calculate_power_regression(double *x, double *y, int num_values);

// Calibrates IR sensor by comparing readings with ultrasonic sensor
void calibrate_ir(oi_t *data) {
    int i = 0;
    // Take readings at different distances
    for (i = 0; i < 7; i++) {
        ping_distances[i] = avg_ping_scan();
        ir_distances[i] = avg_ir_scan();

        move_backward(data, 70, 50);
    }

    calculate_power_regression(ir_distances, ping_distances, 7);
}

// Averages multiple ultrasonic sensor readings
double avg_ping_scan() {
    putty_printf("Ping: ");
    unsigned int sum = 0;
    unsigned int res;
    int i = 0;
    for (i = 0; i < CAL_IR_NUM_ATTEMPTS; i++) {
        res = ping_read();
        sum += res;
        timer_waitMillis(200);
    }

    putty_printf(" %5f\n", sum / CAL_IR_NUM_ATTEMPTS);
    return sum / CAL_IR_NUM_ATTEMPTS;
}

// Averages multiple IR sensor readings
double avg_ir_scan() {
    putty_printf("IR: ");
    int sum = 0;
    int i = 0;
    for (i = 0; i < CAL_IR_NUM_ATTEMPTS; i++) {
        adc_read();

        while (!adc_flag) {
        }

        sum += adc_data;
        putty_printf("%4d,", adc_data);
        adc_flag = 0;
    }

    putty_printf(" %5f\n", sum / CAL_IR_NUM_ATTEMPTS);
    return sum / CAL_IR_NUM_ATTEMPTS;
}

// Calculates power regression, sends resulting formula to UART
void calculate_power_regression(double *x, double *y, int num_values) {
    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_xy = 0.0;
    double sum_x2 = 0.0;

    double logx, logy;
    int i = 0;
    for (i = 0; i < num_values; i++) {
        logx = log(x[i]);
        logy = log(y[i]);

        sum_x += logx;
        sum_y += logy;
        sum_xy += logx * logy;
        sum_x2 += logx * logx;
    }

    // Calculate regression
    double exponent = (num_values * sum_xy - sum_x * sum_y) /
                      (num_values * sum_x2 - sum_x * sum_x);
    double coeff = exp((sum_y - exponent * sum_x) / num_values);

    putty_printf("%.6f * x^{%.6f}\n", coeff, exponent);
}

// Servo calibration using buttons
void calibrate_servo() {
    servo_move(0);

    char string[50];
    int degree_modification = 0;
    int dir = 1;  // 0 for counter, 1 for clockwise
    int cc = 0;

    int correct_zero = 0;
    int correct_one_eighty = 0;

    lcd_puts("1: 1 deg deg 2: 10 deg 3: switch dir 4: set to edge");
    uint8_t res = 0;
    while (cc < 2) {
        res = button_getButton();
        if (res > 0) {
            lcd_clear();
            lcd_puts("1: 1d 2: 10d 3: switch 4: done");
            switch (res) {
                case 1:
                    // Move 1 degree
                    if (dir) {
                        degree_modification -= 1;
                    } else {
                        degree_modification += 1;
                    }
                    servo_move(degree_modification);
                    break;
                case 2:
                    // Move 10 degrees
                    if (dir) {
                        degree_modification -= 10;
                    } else {
                        degree_modification += 10;
                    }
                    servo_move(degree_modification);
                    break;
                case 3:
                    // Switch direction
                    if (dir) {
                        dir = 0;
                    } else {
                        dir = 1;
                    }
                    break;
                case 4:
                    // Move to 0 degrees when in clockwise mode, 180 in counter
                    // Saves values for when done
                    if (cc < 1) {
                        // 0 degrees
                        correct_zero = degree_modification;
                        degree_modification = 180;
                        servo_move(180);
                        dir = 0;
                        cc = 1;
                    } else {
                        // 180 degrees
                        correct_one_eighty = degree_modification;
                        cc = 2;
                    }
                    break;
            }

            if (dir) {
                lcd_puts("Clockwise");
            } else {
                lcd_puts("Counter");
            }
            snprintf(string, 9, "%d ", degree_modification);
            lcd_puts(string);
        }

        timer_waitMillis(10);
    }

    lcd_clear();
    snprintf(string, 20, "0 deg: %3d\r\n", correct_zero);
    lcd_puts(string);
    putty_puts(string, 21);

    snprintf(string, 20, "180 deg: %3d\r\n", correct_one_eighty);
    lcd_puts(string);
    putty_puts(string, 21);

    set_servo_calibation(correct_zero, correct_one_eighty);
}
