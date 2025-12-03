#include "movement.h"

#include "putty.h"
#include "calibration.h"

#include <math.h>
#define PI 3.14159

int move(oi_t *sensor, double distance_mm, int speed_mm, int forward) {
    if (!sensor) {
        return -500; // Return error state if sensor is null
    }
    // Calibration #s for CyBot (see calibration.h)
    distance_mm = distance_mm * STRAIGHT_MULTIPLIER;
    double dist = 0;
    int speed = speed_mm;

    oi_setWheels(speed_mm, speed_mm); // Sets and converts the speed from mm/s
    while (forward ? dist < distance_mm : dist > distance_mm) {
        oi_update(sensor);
        dist += sensor->distance; // Add the new distance travel since last read
        if (forward & (distance_mm - dist < dist * 0.1)) {
            lcd_puts("FORWARD");
            oi_setWheels(15, 15);
            speed = 15;
        } else if (!forward & (distance_mm - dist > dist * 0.1)) {
            lcd_puts("BACKWARD");
            oi_setWheels(-15, -15);
            speed = -15;
        }
        lcd_clear();
        lcd_printf("%.1f / %.1f at speed: %d", dist, distance_mm, speed);
        if (forward && (sensor->bumpLeft || sensor->bumpRight)) {
            break;
        }
        if (sensor->cliffLeft || sensor->cliffFrontLeft || sensor->cliffFrontRight || sensor->cliffFrontRight) {
            break;
        }

    }

    oi_setWheels(0,0); // stop
    oi_update(sensor);
    dist += sensor->distance;

    pos_x += cos(pos_angle * (PI / 180.0)) * dist;
    pos_y += sin(pos_angle * (PI / 180.0)) * dist;
    putty_send_pos();

    lcd_printf("%.1f, %.3f%% error", dist, ((dist-distance_mm)/distance_mm)*100);
    if (forward) {
        return dist; // returns MILLIMETERS
    } else {
        return -dist; // always return positive distance traveled, returns MILLIMETERS
    }
}

int move_forward(oi_t *sensor, int distance_mm, int speed_mm) {
    return move(sensor, distance_mm, speed_mm, 1);
}

int move_backward(oi_t *sensor, int distance_mm, int speed_mm) {
    return move(sensor, -distance_mm, -speed_mm, 0);
}


int turn(oi_t *sensor, int degrees, int speed_mm, int turn_cw) {
    // Calibration #s for CyBot (see calibration.h)
    degrees = (int) (degrees * TURN_MULTIPLIER);
    double ang = sensor->angle;

    oi_setWheels(speed_mm * -1, speed_mm); // Sets and converts the speed from cm/s to mm/s
    while (turn_cw ? -ang < degrees : ang < degrees) {
        oi_update(sensor);
        ang += sensor->angle; // Add the new angle turned since last read
        if (sensor->bumpLeft || sensor->bumpRight) {
            break;
        }
    }
    oi_setWheels(0,0); // stop

    pos_angle += ang;
    putty_send_pos();

    if (turn_cw) {
        return -ang;  // returns DEGREES
    } else {
        return ang; // returns DEGREES
    }
}

int turn_clockwise(oi_t *sensor, int degrees, int speed_mm) {
    return turn(sensor, degrees, speed_mm, 1);
}

int turn_counterclockwise(oi_t *sensor, int degrees, int speed_mm) {
    return turn(sensor, degrees, -speed_mm, 0);
}
