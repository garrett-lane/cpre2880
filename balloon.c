#include "balloon.h"
#include "lcd.h"
#include "open_interface.h"

void balloon_init() {
    SYSCTL_RCGCGPIO_R |= 0x08;      // enable clock GPIOD (page 340)
    GPIO_PORTD_DIR_R   &= ~0x01;         // PD0 as input
    GPIO_PORTD_AFSEL_R &= ~0x01;      // Disable PD0 alternate select (page 671)
    GPIO_PORTD_PCTL_R  &= ~0x000F;       // Clear PD0 pctl (page 688)  also refer to page 650
    GPIO_PORTD_DEN_R   |= 0x01;        // enables PD0

    // interrupt init stuffs
    GPIO_PORTD_IM_R &= ~0x01; // unmask to make changes
    GPIO_PORTD_IS_R &= ~0x01; // set to be edge sensitive
    GPIO_PORTD_IBE_R |= 0x01; // set to trigger on both edges
    GPIO_PORTD_ICR_R |= 0x01; // clear raw interrupt
    GPIO_PORTD_IM_R |= 0x01; // unmask to make changes

    NVIC_EN0_R |= 0x8;

    IntRegister(INT_GPIOD, balloon_handler);
    IntMasterEnable();

    balloon_flag = 0;
}

int read_balloon() {
    return GPIO_PORTD_DATA_R & ~0xFE;
}

void balloon_handler() {
    GPIO_PORTD_ICR_R |= 0x01;
    balloon_flag = 1;
    lcd_printf("Balloon Status: %d", read_balloon());


}
