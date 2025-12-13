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

// Manual navigation mode for CyBot
int run_manual(oi_t *data, cyBOT_Scan_t *scan_data) {
    char temp;
    int left_angle, right_angle;

    while (uart_data != '~') {
        if (uart_flag) {
            temp = uart_data;
            uart_flag = 0;

            switch ((temp & 0xC0) >> 6) {
                case 0b00:
                    // Movement
                    switch ((temp >> 4) & 0x3) {
                        case 0b00:
                            // Foward
                            servo_move(90);
                            move_forward(data, 50, 150);
                            if (data->bumpLeft) {
                                putty_printf("BUMP 0\n");
                            } else if (data->bumpRight) {
                                putty_printf("BUMP 1\n");
                            }
                            break;
                        case 0b01:
                            // Backward
                            move_backward(data, 50, 150);
                            break;
                        case 0b10:
                            // Left
                            turn_counterclockwise(data, 10, 50);
                            break;
                        case 0b11:
                            // Right
                            turn_clockwise(data, 10, 50);
                            break;
                    }
                    break;
                case 0b01:
                    // Scan
                    // Math is on 13 degree intervals so that first 3 bits are 0-90
                    left_angle = 13 * (temp & 0x7);
                    if (left_angle > 90) {
                        left_angle = 90;
                    }
                    // 90 - 180
                    right_angle = 13 * ((temp >> 3) & 0x7) + 90;
                    if (right_angle > 180) {
                        right_angle = 180;
                    }

                    scan_full_arc(scan_data, left_angle, right_angle);
                    find_smallest_objects(scan_sensor_data_array, left_angle, right_angle, scan_data);
                    break;
                case 0b10:
                    // Position
                    // Set base position
                    if (!(temp & 0x20)) {
                        // 67 mm * x
                        pos_x = 67 * (temp & 0x1F);
                        pos_y = 0;
                    } else {
                        // Set angle
                        if (!(temp & 0x10)) {
                            if ((temp & 0x0F) == 0x1) {
                                pos_angle = 0;
                            } else if ((temp & 0x0F) == 0x2) {
                                pos_angle = 90;
                            } else if ((temp & 0x0F) == 0x4) {
                                pos_angle = 180;
                            } else if ((temp & 0x0F) == 0x8) {
                                pos_angle = 270;
                            }
                        } else {
                            if ((temp & 0x0F) == 0x8) {
                                // Invert X
                                pos_x = -pos_x;
                            } else if ((temp & 0x0F) < 0x8) {
                                // Add x centimeters to x
                                pos_x += (temp & 0x07) * 10;
                            } else if ((temp & 0x0F) > 0x8) {
                                // Sub x centimeters to x
                                pos_x -= (temp & 0x07) * 10;
                            }
                        }
                    }
                    putty_send_pos();
                    break;
                case 0b11:
                    // Misc
                    switch (temp & 0x3F) {
                        case 0x01:
                            uart_sendStr("CLEAR\n");
                            break;
                        default: break;
                    }
                    break;
                default: break;
            }

            lcd_printf("Pos x%d y%d d%d", pos_x, pos_y, pos_angle);

            // Always send ACK back to drivers so they can send the next command
            uart_sendStr("ACK\n");
        }
    }

    return 0;
}
