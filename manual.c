#include "adc.h"
#include "button.h"
#include "calibration.h"
#include "cyBot_Scan.h"
#include "driverlib/interrupt.h"
#include "lcd.h"
#include "manual.h"
#include "movement.h"
#include "open_interface.h"
#include "ping.h"
#include "putty.h"
#include "sensor.h"
#include "servo.h"
#include "Timer.h"
#include "uart.h"
#include <inc/tm4c123gh6pm.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

int run_manual(oi_t *data, cyBOT_Scan_t *scan_data) {
    char temp;
    while (uart_data != '~') {
        if (uart_flag) {
            temp = uart_data;

            if (temp == '\r') {
                uart_sendChar('\n');
                goto reset_uart_flag;
            } else if (temp == '\n') {
                goto reset_uart_flag;
            }

            switch (temp) {
                case 'm':
                    scan_full_arc(scan_data, 0, 180);
                    find_smallest_objects(scan_sensor_data_array, 0, 180, scan_data);
                    break;
                case 'q':
                    scan_full_arc(scan_data, 45, 135);
                    find_smallest_objects(scan_sensor_data_array, 45, 135, scan_data);
                    break;
                case 'e':
                    scan_full_arc(scan_data, 75, 105);
                    find_smallest_objects(scan_sensor_data_array, 75, 105, scan_data);
                    break;
                case 'w':
                    move_forward(data, 50, 120);
                    if (data->bumpLeft) {
                        putty_printf("Bump Left - Obstacle Encountered\n");
                    } else if (data->bumpRight) {
                        putty_printf("Bump Right - Obstacle Encountered\n");
                    }
                    break;
                case 's':
                    move_backward(data, 50, 120);
                    break;
                case 'a':
                    turn_counterclockwise(data, 10, 120);
                    break;
                case 'd':
                    turn_clockwise(data, 10, 120);
                    break;
                case 'c':
                    uart_sendStr("CLEAR\n");
                    break;
                case ' ':
                    uart_sendStr("\n");
                    break;
                default: break;
            }

reset_uart_flag:
            uart_flag = 0;
            putty_printf("ACK\n");
        }
    }

    return 0;
}
