/*
*   ping.c
*   Ping sensor interface
*
*
*   @author Garrett Thompson & Sean Krueger
*/


#include "ping.h"
#include "lcd.h"
#include "Timer.h"

int overflows = 0;
int pulse;

void send_pulse();

void ping_init() {
    //initialize registers
    SYSCTL_RCGCGPIO_R |= 0x02;
    SYSCTL_RCGCTIMER_R |= 0x08;

    GPIO_PORTB_DEN_R |= 0x08;
    GPIO_PORTB_AFSEL_R |= 0x08;
    GPIO_PORTB_PCTL_R |= ((0x00007000) & (0xFFF0FFFF));

    TIMER3_CTL_R &= ~0x100;
    TIMER3_CFG_R |= 0x4;
    TIMER3_TBILR_R |= 0xFFFF;
    TIMER3_TBMR_R |= 0x0007;
    TIMER3_TBMR_R &= ~0x0010;
    TIMER3_TBPR_R |= 0xFF;
    TIMER3_CTL_R |= 0xC00;
    TIMER3_ICR_R |= 0x400;
    TIMER3_IMR_R |= 0x400;
    TIMER3_CTL_R |= 0x100;

    NVIC_EN1_R |= 0x10;
    NVIC_PRI9_R &= (0xFFFFFF0F | 0x20);

    IntRegister(INT_TIMER3B, TIMER3B_Handler);
    IntMasterEnable();

}

unsigned int ping_read() {
    //send signal to sensor, wait for isr to collect rising
    //and falling edges and return the pulse width
    state = LOW;
    send_pulse();

    while (state != DONE) { }

    pulse = rising_time - falling_time;
    if (falling_time > rising_time) {
        pulse = (rising_time - falling_time) & 0xFFFFFFFF;
        overflows++;
    }

    // ((delta / clock rate) * speed of light) / round trip
    return (unsigned int) (((pulse / 16000000.0) * 34300) / 2.0);
}

// ISR: Capture rising edge and falling edge time of PING sensor
void TIMER3B_Handler() {
    //store timer values when interrupts are triggered
    if (TIMER3_MIS_R & 0x400) {
        TIMER3_ICR_R |= 0x400;

        if (state == LOW) {
            rising_time = TIMER3_TBR_R;
            state = HIGH;
        } else if (state == HIGH) {
            falling_time = TIMER3_TBR_R;
            state = DONE;
        }
    }
}

void send_pulse() {
    TIMER3_CTL_R &= ~0x100;
    TIMER3_IMR_R &= ~0x400;

    GPIO_PORTB_AFSEL_R &= ~0x08;
    GPIO_PORTB_DIR_R |= 0x08;

    GPIO_PORTB_DATA_R &= ~0x08;
    GPIO_PORTB_DATA_R |= 0x08;

    timer_waitMicros(5);

    GPIO_PORTB_DATA_R &= ~0x08;
    TIMER3_ICR_R |= 0x400;
    GPIO_PORTB_AFSEL_R |= 0x08;

    TIMER3_CTL_R |= 0x100;
    TIMER3_IMR_R |= 0x400;
}

