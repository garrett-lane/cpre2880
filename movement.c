#include "movement.h"

#include "putty.h"
#include "calibration.h"

int move(oi_t *sensor, int distance_mm, int speed_mm, int forward) {
    if (!sensor) {
        return -500; // Return error state if sensor is null
    }
    // Calibration #s for CyBot (see calibration.h)
    distance_mm = (int) (distance_mm * STRAIGHT_MULTIPLIER);
    double dist = 0;

    oi_setWheels(speed_mm, speed_mm); // Sets and converts the speed from mm/s
    while (forward ? dist < distance_mm : dist > distance_mm) {
        oi_update(sensor);
        dist += sensor->distance; // Add the new distance travel since last read
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

    putty_printf("MOVE %d 0\n", (int) dist);
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
    int updated_speed = 0;

    oi_setWheels(speed_mm * -1, speed_mm); // Sets and converts the speed from cm/s to mm/s
    while (turn_cw ? -ang < degrees : ang < degrees) {
        oi_update(sensor);
        ang += sensor->angle; // Add the new angle turned since last read
        if (!updated_speed && (turn_cw ? -ang > (int) degrees * 0.7 : ang > (int) degrees * 0.7)) { // Slow down for the last 30%
            oi_setWheels(-speed_mm / 2, speed_mm / 2);
            updated_speed = 1;
        }
        if (sensor->bumpLeft || sensor->bumpRight) {
            break;
        }
    }
    oi_setWheels(0,0); // stop

    putty_printf("MOVE 0 %d\n", (int) ang);
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
