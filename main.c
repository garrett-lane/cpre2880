#include "adc.h"
#include "button.h"
#include "calibration.h"
#include "cyBot_Scan.h"
#include "driverlib/interrupt.h"
#include "ir.h"
#include "lcd.h"
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
// Dont worry sean, I alphabatized our includes! You're Welcome!

volatile char uart_data;  // Your UART interrupt code can place read data here
#define PI 3.14159

typedef enum movements_e {
    FORWARD,
    BACKWARD,
    CW,
    CCW,
    NONE
} Movements;

typedef struct planned_movement_t {
    Movements move;
    int amount;
    int speed;
} Planned_Movement;

typedef enum states_e {
    IDLE,
    MANUAL,
    AUTO
} States;

Planned_Movement next_move;
int prev_full_scan;

unsigned int dist;
char string[10];

States handle_idle(char val);
States handle_manual(char val, oi_t *data, cyBOT_Scan_t *scan_data);
States handle_auto(char val, oi_t *data, cyBOT_Scan_t *scan_data);

void main()
{
    // Servo Calibration Code
#if CONFIGURE
    oi_t *cal_data = oi_alloc();

    oi_init(cal_data); // should turn the iRobot Create's "Dirt Detect" LED to blue
    lcd_init();
    timer_init();
    ping_init();
    servo_init();
    button_init();
    uart_init(115200);
    uart_interrupt_init();
    adc_init();

    lcd_clear();
    // lcd_puts("Calibrating");

    calibrate_servo();

    servo_move(90);
    timer_waitMillis(500);
    servo_move(45);
    timer_waitMillis(500);
    servo_move(135);
    timer_waitMillis(500);
    servo_move(0);
    timer_waitMillis(500);
    servo_move(180);
    timer_waitMillis(500);

    servo_move(90);
    timer_waitMillis(300);
    char cal_byte ='\0';
    while (uart_data != '~') {
        if (uart_flag) {
            cal_byte = uart_data;

            if (cal_byte == '\n' || cal_byte == '\r') {
                uart_flag = 0;
                continue;
            }

            switch (cal_byte) {
                case 'i':
                    calibrate_ir(cal_data);
                    break;
                case 'o':
                    avg_ir_scan(cal_data);
                    break;
                default: break;
            }

            uart_flag = 0;
        }
    }

    return;
#endif

    oi_t *data = oi_alloc(); // inits serial connect w/ robot; needed before sending commands
    cyBOT_Scan_t scan_data;

    oi_init(data); // should turn the iRobot Create's "Dirt Detect" LED to blue
    lcd_init();
    timer_init();
    servo_init();
    ping_init();
    uart_init(115200);
    uart_interrupt_init();
    adc_init();
    configure_reciever();

    set_servo_calibation(CALIBRATION_0_DEG, CALIBRATION_180_DEG);

    lcd_clear();

    lcd_puts("V0.6");

    States current_state = IDLE;
    next_move.move = NONE;
    prev_full_scan = 0;

    int temp;
    adc_read();
    while (1) {
        if (adc_flag) {
            adc_read();
            adc_flag = 0;
        }

        temp = read_reciever();
        lcd_printf("reciever: %d ", temp);
        timer_waitMillis(10);
    }

    //required end statement
    oi_free(data);
}

States handle_idle(char val) {
    if (!val) {
        return IDLE;
    }
    uart_sendChar(val);
    return IDLE;
}

States handle_manual(char val, oi_t *data, cyBOT_Scan_t *scan_data) {
    switch (val) {
        case 'm':
            scan_full_arc(scan_data, 0, 180);
            find_smallest_objects(scan_sensor_data_array, 0, 180, scan_data);
            return IDLE;
        case 'q':
            scan_full_arc(scan_data, 45, 135);
            find_smallest_objects(scan_sensor_data_array, 45, 135, scan_data);
            return IDLE;
        case 'e':
            scan_full_arc(scan_data, 75, 105);
            find_smallest_objects(scan_sensor_data_array, 75, 105, scan_data);
            return IDLE;
        case 'r':
            adc_read();
            return IDLE;
        case 'w':
            move_forward(data, 50, 120);
            next_move.move = NONE;
            if (data->bumpLeft) {
                putty_printf("Bump Left - Obstacle Encountered\n");
            } else if (data->bumpRight) {
                putty_printf("Bump Right - Obstacle Encountered\n");
            }
            break;
        case 's':
            move_backward(data, 50, 120);
            next_move.move = NONE;
            break;
        case 'a':
            turn_counterclockwise(data, 10, 120);
            next_move.move = NONE;
            prev_full_scan = 0;
            break;
        case 'd':
            turn_clockwise(data, 10, 120);
            next_move.move = NONE;
            prev_full_scan = 0;
            break;
        case 'c':
            uart_sendStr("CLEAR\n");
            break;
        case ' ':
            uart_sendStr("\n");
            break;
        default: break;
    }

    return MANUAL;
}

States handle_auto(char val, oi_t *data, cyBOT_Scan_t *scan_data) {
    if (val != 'h') {
        return IDLE;
    }

    int res;
    if (next_move.move != NONE) {
        switch (next_move.move) {
            case FORWARD:
                res = move_forward(data, next_move.amount, next_move.speed);
                break;
            case CW:
                turn_clockwise(data, next_move.amount, next_move.speed);
                res = next_move.amount;
                break;
            case CCW:
                turn_counterclockwise(data, next_move.amount, next_move.speed);
                res = next_move.amount;
                break;
            default: break;
        }

        if (res < next_move.amount) {
            if (data->bumpLeft) {
                putty_printf("Bump Left - Obstacle Encountered\n");
                move_backward(data, 50, next_move.speed);
                turn_clockwise(data, 90, next_move.speed);
                move_forward(data, 200, next_move.speed);
                turn_counterclockwise(data, (atan(next_move.amount/200.0)*(180.0/PI)) + 90, next_move.speed);
            } else {
                putty_printf("Bump Right - Obstacle Encountered\n");
                move_backward(data, 50, next_move.speed);
                turn_counterclockwise(data, 90, next_move.speed);
                move_forward(data, 200, next_move.speed);
                turn_clockwise(data, (atan(next_move.amount/200.0)*(180.0/PI)) + 90, next_move.speed);
            }
            prev_full_scan = 0;
        }
    }

    Object_t smallest;
    if (!prev_full_scan) {
        scan_full_arc(scan_data, 0, 180);
        smallest = find_smallest_objects(scan_sensor_data_array, 0, 180, scan_data);
        prev_full_scan = 1;
    } else {
        scan_full_arc(scan_data, 45, 135);
        smallest = find_smallest_objects(scan_sensor_data_array, 45, 135, scan_data);
    }

    if (smallest.distance < 9) {
        lcd_clear();
        lcd_puts("Found!");

    } else if (smallest.center_angle < 80 || smallest.center_angle > 100) {
        if (smallest.center_angle > 90) {
            next_move.move = CCW;
            next_move.amount = smallest.center_angle - 90;
            next_move.speed = 75;
        } else {
            next_move.move = CW;
            next_move.amount = 90 - smallest.center_angle;
            next_move.speed = 75;
        }
    } else {
        next_move.move = FORWARD;
        next_move.amount = (int) (smallest.distance * 10) - 100;
        next_move.speed = 120;
    }

    return IDLE;
}
