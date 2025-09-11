#include "open_interface.h"
#include "lcd.h"

#include "movement.h"

int avoid_object(oi_t *sensor) {
    int correct_left;
    if (sensor->bumpLeft > 0) {
        correct_left = 1;
    } else if (sensor->bumpRight > 0) {
        correct_left = 0;
    } else {
        return -100;
    }

    move_backward(sensor, 150, 50);
    if (correct_left) {
        turn_clockwise(sensor, 90, 50);
    } else {
        turn_counterclockwise(sensor, 90, 50);
    }
    move_forward(sensor, 250, 50);
    if (correct_left) {
        turn_counterclockwise(sensor, 90, 50);
    } else {
        turn_clockwise(sensor, 90, 50);
    }

    return 0;
}

void main()
{
    oi_t *sensor_data = oi_alloc(); // inits serial connect w/ robot; needed before sending commands
    oi_init(sensor_data); // should turn the iRobot Create's "Dirt Detect" LED to blue
    lcd_init();

    int target_mm = 2000;
    int traveled = 0;
    int res = 0;
    while (traveled < target_mm) {
        res = move_forward(sensor_data, target_mm - traveled, 100);
        if (res < 0) {
            lcd_printf("Error in move_foward! %d --\n", res);
            break;
        }
        traveled += res;

        if (traveled < target_mm) {
            res = avoid_object(sensor_data);
            if (res < 0) {
                lcd_printf("Error in avoid_object! %d --\n", res);
                break;
            }
            target_mm += 150;
        }
    }


    oi_free(sensor_data);
}
