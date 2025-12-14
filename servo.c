/*
 * servo.c
 *
 *  Created on: Nov 4, 2025
 *      Author: gthomp
 */

#include <Timer.h>
#include <inc/tm4c123gh6pm.h>
#include <servo.h>
#include <stdint.h>

float m = 1;
float b = 0;

// Initializes the servo connected to Port B Pin 5
void servo_init(void) {
    SYSCTL_RCGCGPIO_R |= 0x2;
    SYSCTL_RCGCTIMER_R |= 0x2;

    GPIO_PORTB_DEN_R |= 0x20;
    GPIO_PORTB_DIR_R |= 0x20;
    GPIO_PORTB_AFSEL_R |= 0x20;
    GPIO_PORTB_PCTL_R &= ~0xF00000;
    GPIO_PORTB_PCTL_R |= 0x700000;

    TIMER1_CTL_R &= ~0x100;
    TIMER1_CFG_R = 0x4;
    TIMER1_TBMR_R |= 0xA;  // periodic, pwm en, edge count
    TIMER1_TBPR_R = 0x4;
    TIMER1_TBILR_R = 0xE200;
    TIMER1_TBMATCHR_R &= 0x0;
    TIMER1_TBPMR_R &= 0x0;
    TIMER1_CTL_R |= 0x100;

    current_degrees = 0;
}

// Sets the PWM rate to point the servo in a specific direction
void servo_move(float degrees) {
    float pulse = m * (degrees / 180) + (b / 180.0) +
                  1;  // linear relationship is y = x+1
    int cycles = (320000 - ((pulse / 20) * 320000));

    TIMER1_TBMATCHR_R = cycles & 0xFFFF;
    TIMER1_TBPMR_R = (cycles >> 16) & 0xFF;

    current_degrees = degrees;
    timer_waitMillis(21);
}

// Use pre-defined values to calculate proper offsets for the servo
void set_servo_calibation(float value_z, float value_one_eighty) {
    m = (value_one_eighty - value_z) / 180.0;
    b = value_z;
}
