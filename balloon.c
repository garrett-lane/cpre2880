// Balloon sensor module - detects when robot touches balloon target
// Uses GPIO interrupt on PD0 to signal balloon contact

#include "balloon.h"
#include "lcd.h"
#include "open_interface.h"

// Initialize balloon sensor on GPIO Port D pin 0
// Configures falling-edge interrupt to detect balloon contact
void balloon_init() {
    SYSCTL_RCGCGPIO_R |= 0x08;      // Enable clock for GPIOD (page 340)
    GPIO_PORTD_DIR_R   &= ~0x01;    // Set PD0 as input
    GPIO_PORTD_AFSEL_R &= ~0x01;    // Disable PD0 alternate function select (page 671)
    GPIO_PORTD_PCTL_R  &= ~0x000F;  // Clear PD0 port control (page 688)
    GPIO_PORTD_DEN_R   |= 0x01;     // Enable digital functionality on PD0

    // Configure interrupt settings
    GPIO_PORTD_IM_R &= ~0x01;       // Mask interrupt to make changes
    GPIO_PORTD_IS_R &= ~0x01;       // Set to edge-sensitive
    GPIO_PORTD_IBE_R &= ~0x01;      // Set to NOT trigger on both edges
    GPIO_PORTD_IEV_R &= ~0x01;      // Set to trigger on falling edge
    GPIO_PORTD_ICR_R |= 0x01;       // Clear any pending interrupt
    GPIO_PORTD_IM_R |= 0x01;        // Unmask interrupt to enable

    // Enable interrupt in NVIC
    NVIC_EN0_R |= 0x8;

    // Register interrupt handler
    IntRegister(INT_GPIOD, balloon_handler);
    IntMasterEnable();

    balloon_flag = 0;  // Initialize flag to not detected
}

// Reads current state of balloon sensor
int read_balloon() {
    return GPIO_PORTD_DATA_R && 0x1;
}

// Interrupt handler for balloon detection
// Sets flag when balloon contact is detected
void balloon_handler() {
    GPIO_PORTD_ICR_R |= 0x01;  // Clear interrupt flag
    balloon_flag = 1;           // Signal balloon detected
}
