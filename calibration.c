// Sensor calibration functions for CyBot
// Calibrates IR sensor using power regression and servo alignment

#include "calibration.h"

#include <math.h>

#include "sensor.h"
#include "timer.h"
#include "lcd.h"
#include "servo.h"

#include "adc.h"
#include "ping.h"
#include "putty.h"
#include "movement.h"

// Number of sensor readings to average for calibration
#define CAL_IR_NUM_ATTEMPTS 5.0

// Arrays to store calibration data points
double ping_distances[10];
double ir_distances[10];

// Forward declarations
double avg_ping_scan();
double avg_ir_scan();
void calculate_power_regression(double *x, double *y, int num_values);

// Calibrates IR sensor by comparing readings with ultrasonic sensor
// Takes measurements at 7 distances and calculates power regression
void calibrate_ir(oi_t *data)
{
    int i = 0;
    // Take readings at 7 different distances
    for (i = 0; i < 7; i++)
    {
        ping_distances[i] = avg_ping_scan(); // Ground truth from ultrasonic
        ir_distances[i] = avg_ir_scan();     // IR sensor raw readings

        move_backward(data, 70, 50); // Move back 70mm for next reading
    }

    // Calculate conversion formula from IR readings to actual distance
    calculate_power_regression(ir_distances, ping_distances, 7);
}

// Averages multiple ultrasonic sensor readings
// Returns: Average distance in cm
double avg_ping_scan()
{
    putty_printf("Ping: ");
    unsigned int sum = 0;
    unsigned int res;
    int i = 0;
    for (i = 0; i < CAL_IR_NUM_ATTEMPTS; i++)
    {
        res = ping_read();
        sum += res;
        timer_waitMillis(200);
    }

    putty_printf(" %5f\n", sum / CAL_IR_NUM_ATTEMPTS);
    return sum / CAL_IR_NUM_ATTEMPTS;
}

// Averages multiple IR sensor readings
// Returns: Average IR raw value
double avg_ir_scan()
{
    putty_printf("IR: ");
    int sum = 0;
    int i = 0;
    for (i = 0; i < CAL_IR_NUM_ATTEMPTS; i++)
    {
        adc_read();

        while (!adc_flag)
        {
        }

        sum += adc_data;
        putty_printf("%4d,", adc_data);
        adc_flag = 0;
    }

    putty_printf(" %5f\n", sum / CAL_IR_NUM_ATTEMPTS);
    return sum / CAL_IR_NUM_ATTEMPTS;
}

// Calculates power regression, outputs the numbers we need
void calculate_power_regression(double *x, double *y, int num_values)
{
    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_xy = 0.0;
    double sum_x2 = 0.0;

    double logx, logy;
    int i = 0;
    // Transform to log space to linearize power function
    for (i = 0; i < num_values; i++)
    {
        logx = log(x[i]);
        logy = log(y[i]);

        sum_x += logx;
        sum_y += logy;
        sum_xy += logx * logy;
        sum_x2 += logx * logx;
    }

    // Calculate regression coefficients using least squares
    double exponent = (num_values * sum_xy - sum_x * sum_y) / (num_values * sum_x2 - sum_x * sum_x);
    double coeff = exp((sum_y - exponent * sum_x) / num_values);

    // Output the power regression formula
    putty_printf("%.6f * x^{%.6f}\n", coeff, exponent);
}

// servo calibration using buttons
void calibrate_servo()
{
    servo_move(0);

    char string[50];
    int degree_modification = 0;
    int dir = 1; // Direction: 0 for counter-clockwise, 1 for clockwise
    int cc = 0;  // Calibration step counter

    int correct_zero = 0;       // PWM value for 0 degrees
    int correct_one_eighty = 0; // PWM value for 180 degrees

    lcd_puts("1: 1 deg deg 2: 10 deg 3: switch dir 4: set to edge");
    uint8_t res = 0;
    // Two-step calibration: first align 0 degrees, then 180 degrees
    while (cc < 2)
    {
        res = button_getButton();
        if (res > 0)
        {
            lcd_clear();
            lcd_puts("1: 1d 2: 10d 3: switch 4: done");
            switch (res)
            {
            case 1:
                // Move 1 degree in current direction
                if (dir)
                {
                    degree_modification -= 1;
                }
                else
                {
                    degree_modification += 1;
                }
                servo_move(degree_modification);
                break;
            case 2:
                // Move 10 degrees in current direction
                if (dir)
                {
                    degree_modification -= 10;
                }
                else
                {
                    degree_modification += 10;
                }
                servo_move(degree_modification);
                break;
            case 3:
                // Switch direction (clockwise <-> counter-clockwise)
                if (dir)
                {
                    dir = 0;
                }
                else
                {
                    dir = 1;
                }
                break;
            case 4:
                // Save current position as calibration point
                if (cc < 1)
                {
                    // First calibration point: 0 degrees
                    correct_zero = degree_modification;
                    degree_modification = 180;
                    servo_move(180);
                    dir = 0;
                    cc = 1;
                }
                else
                {
                    // Second calibration point: 180 degrees
                    correct_one_eighty = degree_modification;
                    cc = 2;
                }
                break;
            }

            // Display current direction and position
            if (dir)
            {
                lcd_puts("Clockwise");
            }
            else
            {
                lcd_puts("Counter");
            }
            snprintf(string, 9, "%d ", degree_modification);
            lcd_puts(string);
        }

        timer_waitMillis(10);
    }

    // Display and save calibration results
    lcd_clear();
    snprintf(string, 20, "0 deg: %3d\r\n", correct_zero);
    lcd_puts(string);
    putty_puts(string, 21);

    snprintf(string, 20, "180 deg: %3d\r\n", correct_one_eighty);
    lcd_puts(string);
    putty_puts(string, 21);

    // Apply calibration values for future servo operations
    set_servo_calibation(correct_zero, correct_one_eighty);
}
