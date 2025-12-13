// Autonomous navigation mode for CyBot
// Scans for objects, finds gaps, and navigates autonomously

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

// Scans IR sensor data to detect and identify objects in front of the bot
// Returns: Number of objects found
int find_objects(cyBOT_Scan_t data_array[], int start_angle, int end_angle, Object_t objects[])
{
    int objStartIndex = 0;

    int in_object = 0; // boolean for in object

    int i;
    int j = 0;
    objects[j].distance = 10;
    objects[j].end = 0;
    objects[j].start = 0;
    j++;

    // Loop through scan data and find objects, store them in objects arr
    for (i = 1; i < 91; i++)
    {
        // if distance is < 8% of prev. value, start object
        if (!in_object && data_array[i].IR_raw_val < 150 && data_array[i].IR_raw_val < 0.85 * data_array[i - 1].IR_raw_val)
        {
            objects[j].start = start_angle + (i * 2); // starting degree
            in_object = 1;
            objStartIndex = i;
        }
        else if (in_object && data_array[i].IR_raw_val > 1.15 * data_array[i - 1].IR_raw_val)
        { // if distance is > 115% of prev. data item, end object
            objects[j].distance = data_array[(i + objStartIndex) / 2].sound_dist;
            objects[j].end = start_angle + ((i - 1) * 2);
            objects[j].center_angle = (objects[j].start + objects[j].end) / 2.0;
            objects[j].linear_width = objects[j].distance * ((objects[j].end - objects[j].start) * (PI / 180.0));
            // Sends only objects we deem real to the gui, filters out false reads
            if (objects[j].linear_width > 5 && objects[j].linear_width < 20 && objects[j].distance < 300)
            {
                putty_send_object(&objects[j]);
                j++;
            }
            in_object = 0;
            // shouldnt have this many objs
            if (j > 11)
            {
                break;
            }
        }
    }
    objects[j].distance = 501.0;
    objects[j].end = 180;
    objects[j].start = 180;
    return j + 1;
}

// Moves robot forward while monitoring for obstacles
// Detects tape and bumps, takes EVASIVE (IF EVASION NOT POSSIBLE, EXECUTE) action
void progress(oi_t *data)
{
    move_forward(data, 75, 80);
    // Detects black and white tape on the left, and directs away from it
    if (data->cliffLeft || data->cliffFrontLeft ||
        data->cliffLeftSignal > 2600 || data->cliffFrontLeftSignal > 2600)
    {
        lcd_puts("CLIFF");
        move_backward(data, 125, 80);
        turn_clockwise(data, 90, 50);
        return;
    }
    // Detects black and white tape on the right, and directs away from it
    if (data->cliffRight || data->cliffFrontRight ||
        data->cliffRightSignal > 2600 || data->cliffFrontRightSignal > 2600)
    {
        lcd_puts("CLIFF");
        move_backward(data, 125, 80);
        turn_counterclockwise(data, 90, 50);
        return;
    }
    // Detects bump on the left
    if (data->bumpLeft)
    {
        lcd_puts("BUMP");
        move_backward(data, 125, 80);
        turn_clockwise(data, 120, 50);
        timer_waitMillis(500);
        return;
    }
    // Detects bump on the right
    if (data->bumpRight)
    {
        lcd_puts("BUMP");
        move_backward(data, 125, 80);
        turn_counterclockwise(data, 120, 50);
        timer_waitMillis(500);
        return;
    }
}

// Main autonomous navigation function
// Continuously scans for objects, finds navigable gaps, and moves toward them
// Stops when balloon sensor is triggered
int run_auto(oi_t *data, cyBOT_Scan_t *scan_data)
{
    const float ROBOT_WIDTH = 32.0;
    int found_gap = 0;
    int gap_center_angle = 90;
    int obj_count;
    int i;
    int right_start;
    int left_end;
    int gap_degrees;
    float theta_rad;
    float d_left_edge;
    float d_right_edge;
    float d;
    int left;
    int right;
    int center;
    float gap_width;
    char temp;

    // Wait for GUI to set robot's initial position
    // receives encoded position and angle data via UART
    while (uart_data != 0b11000010)
    {
        if (uart_flag && uart_data != 0b11000010)
        {
            temp = uart_data;
            uart_flag = 0;
            if (!(temp & 0x20))
            {
                // 130 mm * x
                pos_x = 67 * (temp & 0x1F);
                pos_y = 0;
            }
            else
            {
                // Set angle
                if (!(temp & 0x10))
                {
                    if ((temp & 0x0F) == 0x1)
                    {
                        pos_angle = 0;
                    }
                    else if ((temp & 0x0F) == 0x2)
                    {
                        pos_angle = 90;
                    }
                    else if ((temp & 0x0F) == 0x4)
                    {
                        pos_angle = 180;
                    }
                    else if ((temp & 0x0F) == 0x8)
                    {
                        pos_angle = 270;
                    }
                }
                else
                {
                    if ((temp & 0x0F) == 0x8)
                    {
                        // Invert X
                        pos_x = -pos_x;
                    }
                    else if ((temp & 0x0F) < 0x8)
                    {
                        // Add x centimeters to x
                        pos_x += (temp & 0x07) * 10;
                    }
                    else if ((temp & 0x0F) > 0x8)
                    {
                        // Sub x centimeters to x
                        pos_x -= (temp & 0x07) * 10;
                    }
                }
            }
            putty_send_pos();
            uart_sendStr("ACK\n");
        }
    }

    // Main auto loop - runs until balloon is detected
    while (!balloon_flag)
    {
        // 180 scan in front of robot
        scan_full_arc(scan_data, 0, 180);
        lcd_clear();

        // find objs
        Object_t objects[15];
        obj_count = find_objects(scan_sensor_data_array, 0, 180, objects);
        if (balloon_flag)
            break;

        // Calculate gaps between objects to find possible paths
        gap_center_angle = 0;
        found_gap = 0;
        // If multiple objects detected, look for gaps between them
        if (obj_count > 3)
        {
            for (i = 0; i < obj_count - 1; i++)
            {
                right_start = objects[i].end;
                left_end = objects[i + 1].start;
                gap_degrees = left_end - right_start;
                if (gap_degrees > 0)
                {
                    // Convert angle to radians
                    theta_rad = gap_degrees * (M_PI / 180.0);

                    // Get distances at both edges of the gap
                    d_left_edge = objects[i + 1].distance;
                    d_right_edge = objects[i].distance;

                    // Use the farther edge for safer navigation
                    d = fmax(d_left_edge, d_right_edge);

                    // Calculate actual gap width using trigonometry
                    gap_width = 2.0 * d * sin(theta_rad / 2.0);

                    // Check if gap is wide enough for robot to fit
                    if (gap_width >= ROBOT_WIDTH)
                    {
                        found_gap = 1;
                        gap_center_angle = objects[i].end + gap_degrees / 2.0;
                        break;
                    }
                }
            }
        }
        if (balloon_flag)
            break;

        // drive toward the found gap
        if (found_gap)
        {
            if (gap_center_angle < 90)
            {
                lcd_printf("Going to gap: %d", gap_center_angle);
                turn_clockwise(data, 90 - gap_center_angle, 50);
            }
            else
            {
                turn_counterclockwise(data, gap_center_angle - 90, 50);
            }
            progress(data);

            // If only one object detected, go around it on the larger side
        }
        else if (obj_count == 3)
        { // 3 because of fake boundary objects at 0 and 180
            center = 0;
            left = objects[1].start;
            right = 180 - objects[1].end;
            // Check which side is bigger and aim for the center of that
            if (left > right)
            {
                center = left / 2;
            }
            else
            {
                center = (right / 2) + objects[1].end;
            }
            if (center < 90)
            {
                lcd_printf("One object found, going right: %d", center);
                turn_clockwise(data, 90 - center, 50);
            }
            else
            {
                lcd_printf("One object found, going left: %d", center);
                turn_counterclockwise(data, center - 90, 50);
            }
            progress(data);
        }
        else if (obj_count == 2)
        {
            // No objects detected; go forward
            progress(data);
            lcd_puts("No object");
        }
        else
        {
            // No navigable gap found; turn 90 degrees and scan again
            lcd_puts("No gap");
            turn_counterclockwise(data, 90, 50);
        }
    }

    // Balloon detected because exited loop
    putty_printf("BALLOON\n");

    // Hedwig's Theme
    unsigned char noteCount = 14;
    unsigned char notes[14] = {81, 74, 77, 76, 74, 81, 79, 76, 74, 77, 76, 73, 75, 69};
    unsigned char durations[14] = {32, 48, 16, 32, 64, 32, 96, 96, 48, 16, 32, 64, 32, 96};
    oi_loadSong(0, noteCount, notes, durations);
    oi_play_song(0);

    return 0;
}
