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

Planned_Movement next_move;
int prev_full_scan;
int find_objects(cyBOT_Scan_t data_array[], int start_angle, int end_angle, Object_t objects[]){//cyBOT_Scan_t *scan_data
    //float dist = 999.9;
    //int start_deg = 0; // current object angle
    //int end_deg = 0;
    int objStartIndex = 0;

    int in_object = 0;


    int i = 1;
    int j = 0;
    objects[j].distance = 501;
    objects[j].end = 0;
    objects[j].start = 0;
    j++;
    for (i = 1; i < 91; i++) {
        // if distance is < 8% of prev. value, start object
        if (!in_object && data_array[i].IR_raw_val < 150 && data_array[i].IR_raw_val < 0.85 * data_array[i-1].IR_raw_val) {
            objects[j].start = start_angle + (i * 2); // starting degree
            in_object = 1;
            objStartIndex = i;
        } else if (in_object && data_array[i].IR_raw_val > 1.15 * data_array[i-1].IR_raw_val) { // if distance is > 115% of prev. data item, end object
            objects[j].distance = data_array[(i-objStartIndex) / 2].sound_dist;
            objects[j].end = start_angle + ((i-1) * 2);
            objects[j].center_angle = (objects[j].start + objects[j].end) / 2.0;
            objects[j].linear_width = objects[j].distance * ((objects[j].end - objects[j].start) * (PI / 180.0));
            putty_send_object(&objects[j]);
            j++;
            in_object = 0;
            if(j>11){
                break;
            }
        }
    }
    objects[j].distance = 501.0;
    objects[j].end = 180;
    objects[j].start = 180;
    return j;
}
void progress(oi_t *data){
    int d = move_forward(data, 100, 80);

    if (d < 100)
    {
        if (data->cliffLeft || data->cliffFrontLeft ||
            data->cliffLeftSignal > 2600 || data->cliffFrontLeftSignal > 2600)
        {
            move_backward(data, 100, 80);
            turn_clockwise(data, 45, 80);
        }
        if (data->cliffRight || data->cliffFrontRight ||
            data->cliffRightSignal > 2600 || data->cliffFrontRightSignal > 2600)
        {
            move_backward(data, 100, 80);
            turn_counterclockwise(data, 45, 80);
        }
        if (data->bumpLeft)
        {
            move_backward(data, 100, 80);
            turn_clockwise(data, 90, 80);
        }
        if (data->bumpRight)
        {
            move_backward(data, 100, 80);
            turn_counterclockwise(data, 90, 80);
        }
    }
}
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
    lcd_puts("Calibrating");

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

    set_servo_calibation(CALIBRATION_0_DEG, CALIBRATION_180_DEG);

    lcd_clear();

    lcd_puts("V0.8");

#if MANUAL
    run_manual(data, &scan_data);
#elif AUTO
    // PUT AUTO CODE HERE
    // Position
    // Set base position
    char temp;
    while(uart_data != 0b11000010){
        if (uart_flag) {
            temp = uart_data;
            uart_flag = 0;
            if (!(temp & 0x20)) {
                // 130 mm * x
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
                        pos_x += temp & 0x07;
                    } else if ((temp & 0x0F) > 0x8) {
                        // Sub x centimeters to x
                        pos_x -= temp & 0x07;
                    }
                }
            }


            putty_send_pos();
            uart_sendStr("ACK\n");
        }
    }
    const float ROBOT_WIDTH = 32.0;
    int found_gap = 0;
    int gap_center_angle = 90;
	while (1)
{
    //1. Full scan
    scan_full_arc(&scan_data, 0, 180);
    lcd_clear();
    //2. Detect objects
    Object_t objects[12];
    int obj_count = find_objects(scan_sensor_data_array, 0, 180, objects);

    //3. Use math find gaps

    found_gap = 0;
    int i;
    for (i = 0; i < obj_count - 1; i++)
    {
        int left_end  = objects[i].end;
        int right_start = objects[i+1].start;

        int gap_degrees = right_start - left_end;

        if (gap_degrees > 0)
        {
            float gap_angle_rad = gap_degrees * (M_PI / 180.0);

            
            float gap_distance = fmin(objects[i].distance, objects[i+1].distance);

           
            float gap_width = gap_distance * gap_angle_rad;

            if (gap_width >= ROBOT_WIDTH)
            {
                found_gap = 1;
                lcd_puts("found gap");
                gap_center_angle = left_end + gap_degrees / 2;
                lcd_printf("%d", gap_center_angle);
                break;
            }
        }
    }

    //4. Shoot the gap, first valid one
    if (found_gap)
    {
        
        if (gap_center_angle < 90)
            turn_clockwise(data, 90 - gap_center_angle, 80);
        else
            turn_counterclockwise(data, gap_center_angle - 90, 80);

        progress(data);
        
    }else if(obj_count == 2){
        progress(data);
    }
    else
    {
        //5. No gap, 180°
        turn_counterclockwise(data, 180, 80);
    }
}
#endif

    //required end statement
    oi_free(data);
}
