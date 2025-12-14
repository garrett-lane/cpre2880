#include <inc/tm4c123gh6pm.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "Timer.h"
#include "adc.h"
#include "auto.h"
#include "balloon.h"
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
#include "uart.h"
// Dont worry sean, I alphabatized our includes! You're Welcome!

volatile char uart_data;  // Your UART interrupt code can place read data here

typedef enum movements_e { FORWARD, BACKWARD, CW, CCW, NONE } Movements;

typedef struct planned_movement_t {
    Movements move;
    int amount;
    int speed;
} Planned_Movement;

Planned_Movement next_move;
int prev_full_scan;

void main() {
    // Servo Calibration Code
#if CONFIGURE
    oi_t *cal_data = oi_alloc();

    oi_init(
        cal_data);  // should turn the iRobot Create's "Dirt Detect" LED to blue
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
    char cal_byte = '\0';
    // Waits for putty
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
                default:
                    break;
            }

            uart_flag = 0;
        }
    }

    return;
#endif

    oi_t *data = oi_alloc();  // inits serial connect w/ robot; needed before
                              // sending commands
    cyBOT_Scan_t scan_data;

    oi_init(data);  // should turn the iRobot Create's "Dirt Detect" LED to blue
    lcd_init();
    timer_init();
    servo_init();
    ping_init();
    uart_init(115200);
    uart_interrupt_init();
    adc_init();
    balloon_init();

    set_servo_calibation(CALIBRATION_0_DEG, CALIBRATION_180_DEG);

    lcd_clear();
    lcd_puts("v1.3");

    // Runs Auto or Manual based on compiler flag, or exists and clears blue LED
#if MANUAL
    run_manual(data, &scan_data);
#elif AUTO
    run_auto(data, &scan_data);
#endif

    // required end statement
    oi_free(data);
}
