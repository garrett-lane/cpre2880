#include "movement.h"

#include "putty.h"
#include "calibration.h"

#include <math.h>
#define PI 3.14159

// move roboto forward and backward
// Returns Distance traveled in mm (positive or negative)
int move(oi_t *sensor, double distance_mm, int speed_mm, int forward)
{
    if (!sensor)
    {
        return -500; // Return error state if sensor no work
    }
    // Apply calibration multiplier to compensate for wheel slippage
    distance_mm = distance_mm * STRAIGHT_MULTIPLIER;
    double dist = 0;

    oi_setWheels(speed_mm, speed_mm); // Sets and converts the speed from mm/s
    // Movement loop - continues until target distance reached
    while (forward ? dist < distance_mm : dist > distance_mm)
    {
        oi_update(sensor);
        dist += sensor->distance; // Add the new distance travel since last read

        // Slow down when approaching target distance for more accurate stopping
        if (forward & (distance_mm - dist < dist * 0.1))
        {
            oi_setWheels(15, 15);
        }
        else if (!forward & (distance_mm - dist > dist * 0.1))
        {
            oi_setWheels(-15, -15);
        }

        // Stop if bump sensor triggered
        if (forward && (sensor->bumpLeft || sensor->bumpRight))
        {
            break;
        }

        // Detect cliff/tape boundaries and report to GUI
        // 1st digit is black or white, second digit is left or right
        if (sensor->cliffLeft || sensor->cliffFrontLeft)
        {
            putty_printf("TAPE 0 0");
            break;
        }
        if (sensor->cliffFrontRight || sensor->cliffFrontRight)
        {
            putty_printf("TAPE 0 1");
            break;
        }
        if (sensor->cliffLeftSignal > 2600 || sensor->cliffFrontLeftSignal > 2600)
        {
            putty_printf("TAPE 1 0");
            break;
        }
        if (sensor->cliffRightSignal > 2600 || sensor->cliffFrontRightSignal > 2600)
        {
            putty_printf("TAPE 1 1");
            break;
        }
    }

    oi_setWheels(0, 0); // stop!
    oi_update(sensor);
    dist += sensor->distance;

    // Update global position based on movement
    pos_x += cos(pos_angle * (PI / 180.0)) * dist;
    pos_y += sin(pos_angle * (PI / 180.0)) * dist;
    putty_send_pos();

    // lcd_printf("%.1f, %.3f%% error", dist, ((dist-distance_mm)/distance_mm)*100);
    if (forward)
    {
        return dist; // returns MILLIMETERS
    }
    else
    {
        return -dist; // always return positive distance traveled, returns MILLIMETERS
    }
}

// Move robot forward by specified distance
int move_forward(oi_t *sensor, int distance_mm, int speed_mm)
{
    return move(sensor, distance_mm, speed_mm, 1);
}

// Move robot backward by specified distance
int move_backward(oi_t *sensor, int distance_mm, int speed_mm)
{
    return move(sensor, -distance_mm, -speed_mm, 0);
}

// rotate robot in place
// Returns Degrees turned
int turn(oi_t *sensor, int degrees, int speed_mm, int turn_cw)
{
    // Apply calibration multiplier to correct turning accuracy
    degrees = (int)(degrees * TURN_MULTIPLIER);
    double ang = sensor->angle;

    oi_setWheels(speed_mm * -1, speed_mm); // Sets and converts the speed from cm/s to mm/s
    // Turning loop - continues until target angle reached
    while (turn_cw ? -ang < degrees : ang < degrees)
    {
        oi_update(sensor);
        ang += sensor->angle; // Add the new angle turned since last read
        if (sensor->bumpLeft || sensor->bumpRight)
        {
            break;
        }
    }
    oi_setWheels(0, 0); // stop

    // Update global angle
    pos_angle += ang;
    putty_send_pos();

    if (turn_cw)
    {
        return -ang; // returns DEGREES
    }
    else
    {
        return ang; // returns DEGREES
    }
}

// Turn robot clockwise by specified degrees
int turn_clockwise(oi_t *sensor, int degrees, int speed_mm)
{
    return turn(sensor, degrees, speed_mm, 1);
}

// Turn robot counter-clockwise by specified degrees
int turn_counterclockwise(oi_t *sensor, int degrees, int speed_mm)
{
    return turn(sensor, degrees, -speed_mm, 0);
}
