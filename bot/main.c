#include "open_interface.h"
#include "lcd.h"
#include "cyBot_Scan.h"
#include "cyBot_uart.h"
#include "timer.h"

#include "calibration.h"
#include "movement.h"
#include "putty.h"
#include "sensor.h"

// Method Prototypes
float evaluate_data(float data_array[], int arrLength);

void scan(cyBOT_Scan_t *scan_data, oi_t *data);

void main()
{
    // Servo Calibration Code
#if CONFIGURE
    timer_init();
    lcd_init();
    cyBOT_init_Scan(0b0011);
    cyBot_uart_init();

    cyBOT_SERVRO_cal_t cal = cyBOT_SERVO_cal();
    putty_printf("Left: %d\n", cal.left);
    putty_printf("Right: %d\n", cal.right);

    return;
#endif

    oi_t *data = oi_alloc(); // inits serial connect w/ robot; needed before sending commands
    cyBOT_Scan_t scan_data;

    oi_init(data); // should turn the iRobot Create's "Dirt Detect" LED to blue
    lcd_init();
    timer_init();
    cyBot_uart_init();
    // Bits (Useless, IR Sensor, Ping Sensor, Servo)
    cyBOT_init_Scan(0b0011);

    // Calibration #s for CyBot (see calibration.h)
    left_calibration_value = LEFT_SERVO_CALIBRATION;
    right_calibration_value = RIGHT_SERVO_CALIBRATION;

    lcd_clear();

    char byte ='\0';
    while (byte != '~') {
        byte = cyBot_getByte();
        if (byte == '\n' || byte == '\r') {
            continue;
        }

        lcd_putc(byte);

        switch (byte) {
            case 'm':
                scan(&scan_data, data);
                break;
            case 'w':
                move_forward(data, 50, 250);
                break;
            case 's':
                move_backward(data, 50, 250);
                break;
            case 'a':
                turn_counterclockwise(data, 15, 250);
                break;
            case 'd':
                turn_clockwise(data, 15, 250);
                break;
            case 'c':
                putty_send_string("CLEAR\n", 6);
                break;
            default:  break;
        }
    }



    //required end statement
    oi_free(data);
}

void scan(cyBOT_Scan_t *scan_data, oi_t *data) {
    float ptToDeg = 0;
    char debugString[10];

    cyBOT_Scan(0, scan_data);  // set to zero to have correct data
    timer_waitMillis(1000);
    putty_printf("STARTSCAN 91\n");
    scan_range(0, 180, 2, scan_data, process_noisy_data);
    timer_waitMillis(200);
    putty_printf("ENDSCAN\n");

#if DEBUG
    int i = 0;
    putty_printf("STARTSCAN 91\n");
    for (i = 0; i < 91; i++) {
        putty_printf("%3d\t\t%10f\n", i * 2, scan_sensor_data_array[i]);
    }
    timer_waitMillis(200);
    putty_printf("ENDSCAN\n");
#endif

    ptToDeg = evaluate_data(scan_sensor_data_array, 91);
    sprintf(debugString, "%.3lf", ptToDeg);
    lcd_puts(debugString);
    cyBOT_Scan(ptToDeg, scan_data);
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
                putty_send_object(&current_object);
                timer_waitMillis(200);
            }

            if (current_object.linear_width < smallest_obj_lin_width) {
                smallest_obj_lin_width = current_object.linear_width;
                smallest_obj_ctr_angle = current_object.center_angle;
            }
        }
    }

    return smallest_obj_ctr_angle;
}
