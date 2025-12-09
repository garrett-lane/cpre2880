#include "adc.h"
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
#include "Timer.h"
#include "uart.h"
#include <inc/tm4c123gh6pm.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define PI 3.14159
/*
Used with the scan to find objects in front of the bot
*/
int find_objects(cyBOT_Scan_t data_array[], int start_angle, int end_angle, Object_t objects[]){
    int objStartIndex = 0;

    int in_object = 0; //Flag to make sure objects get closed properly

    int i;
    int j = 0;
	//Make an arbitrary object at 0 degrees
    objects[j].distance = 10;
    objects[j].end = 0;
    objects[j].start = 0;
    j++;
	
	//Loop through scan data and find objects, store them in objects arr
    for (i = 1; i < 91; i++) {
        // if distance is < 8% of prev. value, start object
        if (!in_object && data_array[i].IR_raw_val < 150 && data_array[i].IR_raw_val < 0.85 * data_array[i-1].IR_raw_val) {
            objects[j].start = start_angle + (i * 2); // starting degree
            in_object = 1;
            objStartIndex = i;
        } else if (in_object && data_array[i].IR_raw_val > 1.15 * data_array[i-1].IR_raw_val) { // if distance is > 115% of prev. data item, end object
            objects[j].distance = data_array[(i + objStartIndex) / 2].sound_dist;
            objects[j].end = start_angle + ((i-1) * 2);
            objects[j].center_angle = (objects[j].start + objects[j].end) / 2.0;
            objects[j].linear_width = objects[j].distance * ((objects[j].end - objects[j].start) * (PI / 180.0));
			//Sends only objects we deem real to the gui, filters out false reads
            if (objects[j].linear_width > 5 && objects[j].linear_width < 20 && objects[j].distance < 300) {
				putty_send_object(&objects[j]);
                j++;
            }
            in_object = 0;
			//Won't be this many objects
            if(j>11){
                break;
            }
        }
    }
	//Make second arbitrary object at 180 degrees
    objects[j].distance = 501.0;
    objects[j].end = 180;
    objects[j].start = 180;
    return j+1;
}

/*
Move forward function
*/
void progress(oi_t *data) {
   move_forward(data, 75, 80);
   //Detects black and white tape on the left, and maneuvers away from it
   if (data->cliffLeft || data->cliffFrontLeft ||
       data->cliffLeftSignal > 2600 || data->cliffFrontLeftSignal > 2600)
   {
       lcd_puts("CLIFF");
       move_backward(data, 125, 80);
       turn_clockwise(data, 90, 50);
       return;
   }
   //Detects black and white tape on the right, and maneuvers away from it
   if (data->cliffRight || data->cliffFrontRight ||
       data->cliffRightSignal > 2600 || data->cliffFrontRightSignal > 2600)
   {
       lcd_puts("CLIFF");
       move_backward(data, 125, 80);
       turn_counterclockwise(data, 90, 50);
       return;
   }
   //Detects bump on the left
   if (data->bumpLeft)
   {
       lcd_puts("BUMP");
       move_backward(data, 125, 80);
       turn_clockwise(data, 120, 50);
       timer_waitMillis(500);
       return;
   }
   //Detects bump on the right
   if (data->bumpRight)
   {
       lcd_puts("BUMP");
       move_backward(data, 125, 80);
       turn_counterclockwise(data, 120, 50);
       timer_waitMillis(500);
       return;
   }
}

/*
Actual function called in main
*/
int run_auto(oi_t *data, cyBOT_Scan_t *scan_data) {
    const float ROBOT_WIDTH = 32.0;
    int found_gap = 0;
    int gap_center_angle = 90;
    int obj_count;
    int i;
    int right_start;
    int left_end;
    int gap_degrees;
   // float gap_angle_rad;  old stuff did not work
   // float gap_distance;
    float theta_rad; // new math good, hopefully
    float d_left_edge;
    float d_right_edge;
    float d;
    int left;
    int right;
    int center;
    float gap_width;
    char temp;
	
	//Sean gui stuff 
    while(uart_data != 0b11000010){
        if (uart_flag && uart_data != 0b11000010) {
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
                        pos_x += (temp & 0x07) * 10;
                    } else if ((temp & 0x0F) > 0x8) {
                        // Sub x centimeters to x
                        pos_x -= (temp & 0x07) * 10;
                    }
                }
            }
            putty_send_pos();
            uart_sendStr("ACK\n");
        }
    }
	//While the balloon has a connection
    while (!balloon_flag) {
        //1. Full scan 180 in front
        scan_full_arc(scan_data, 0, 180);
        lcd_clear();
        //2. Detect objects
        Object_t objects[15];
        obj_count = find_objects(scan_sensor_data_array, 0, 180, objects);
        if (balloon_flag) break;
        //3. Use math to find gaps
        gap_center_angle = 0;
        found_gap = 0;
        if (obj_count > 3) {
            for (i = 0; i < obj_count-1; i++) {
                right_start = objects[i].end;
                left_end  = objects[i+1].start;
                gap_degrees = left_end - right_start;
                if (gap_degrees > 0) {

                    theta_rad = gap_degrees * (M_PI / 180.0);

                    //Distances at the edges, instead of center
                    d_left_edge  = objects[i+1].distance;
                    d_right_edge = objects[i].distance;

                    //Use the farther edge instead of previous closer one
                    d = fmax(d_left_edge, d_right_edge);

                    //Better width calc hopefully
                    gap_width = 2.0 * d * sin(theta_rad / 2.0);

                    if (gap_width >= ROBOT_WIDTH) {
                        found_gap = 1;
                        gap_center_angle = objects[i].end + gap_degrees / 2.0;
                        break;
                    }
                }
            }
        }
        if (balloon_flag) break;
        //4. Enter the first valid gap
        if (found_gap) {
            if (gap_center_angle < 90) {
                lcd_printf("Going to gap: %d", gap_center_angle);
                turn_clockwise(data, 90 - gap_center_angle, 50);
            } else {
                turn_counterclockwise(data, gap_center_angle - 90, 50);
            }
            progress(data);

        //5. Enter larger side (left or right) if only one object
        } else if (obj_count == 3) {//3 because fake objects at 0 and 180
            center = 0;
            left = objects[1].start;
            right = 180 - objects[1].end;
			//Check which side is bigger and aim for the center of that
            if (left > right) {
                center = left / 2;

            } else {
                center = (right / 2) + objects[1].end;

            }
            if(center < 90){
                lcd_printf("One object found, going right: %d", center);
                turn_clockwise(data, 90 - center, 50);
            }else{
                lcd_printf("One object found, going left: %d", center);
                turn_counterclockwise(data, center - 90, 50);
            }
            progress(data);
        }else if(obj_count == 2){
            progress(data);
            lcd_puts("No object");
        } else {
            //5. No gap, turn 90 degrees and try again
            lcd_puts("No gap");
            turn_counterclockwise(data, 90, 50);
        }

    }
	//When the balloon "pops" or looses connection, play the amazing music
    putty_printf("BALLOON\n");

    // Hedwig's Theme
    unsigned char noteCount = 14;
    unsigned char notes[14] = {81, 74, 77, 76, 74, 81, 79, 76, 74, 77, 76, 73, 75, 69};
    unsigned char durations[14] = {32, 48, 16, 32, 64, 32, 96, 96, 48, 16, 32, 64, 32, 96 };
    oi_loadSong(0, noteCount, notes, durations);
    oi_play_song(0);



    return 0;
}
