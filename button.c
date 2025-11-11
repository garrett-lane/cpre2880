/*
 * button.c
 *
 *  Created on: Jul 18, 2016
 *      Author: Eric Middleton, Zhao Zhang, Chad Nelson, & Zachary Glanz.
 *
 *  @edit: Lindsey Sleeth and Sam Stifter on 02/04/2019
 *  @edit: Phillip Jone 05/30/2019: Mearged Spring 2019 version with Fall 2018
 */
 


//The buttons are on PORTE 3:0
// GPIO_PORTE_DATA_R -- Name of the memory mapped register for GPIO Port E, 
// which is connected to the push buttons
#include "button.h"

int prev_b1 = 0;
int prev_b2 = 0;
int prev_b3 = 0;
int prev_b4 = 0;


/**
 * Initialize PORTE and configure bits 0-3 to be used as inputs for the buttons.
 */
void button_init() {
	static uint8_t initialized = 0;

	//Check if already initialized
	if(initialized){
		return;
	}
	
	// Reading: To initialize and configure GPIO PORTE, visit pg. 656 in the 
	// Tiva datasheet.
	
	// Follow steps in 10.3 for initialization and configuration. Some steps 
	// have been outlined below.
	
	// Ignore all other steps in initialization and configuration that are not 
	// listed below. You will learn more about additional steps in a later lab.

	// 1) Turn on PORTE system clock, do not modify other clock enables
	SYSCTL_RCGCGPIO_R |= 0x10;

	// 2) Set the buttons as inputs, do not modify other PORTE wires
	GPIO_PORTE_DIR_R &= ~(0xF);
	
	// 3) Enable digital functionality for button inputs, 
	//    do not modify other PORTE enables
	GPIO_PORTE_DEN_R |= 0xF;

	
	initialized = 1;
}



/**
 * Initialize and configure PORTE interupts
 */
void init_button_interrupts() {
    // In order to configure GPIO ports to detect interrupts, you will need to visit pg. 656 in the Tiva datasheet.
    // Notice that you already followed some steps in 10.3 for initialization and configuration of the GPIO ports in the function button_init().
    // Additional steps for setting up the GPIO port to detect interrupts have been outlined below.


    // 1) Mask the bits for pins 0-3
    GPIO_PORTE_IM_R &= ~(0x0F);

    // 2) Set pins 0-3 to use edge sensing
    GPIO_PORTE_IS_R &= ~(0x0F);

    // 3) Set pins 0-3 to use both edges. We want to update the LCD
    //    when a button is pressed, and when the button is released.
    GPIO_PORTE_IBE_R |= 0x0F;

    // 4) Clear the interrupts
    GPIO_PORTE_ICR_R = 0xFF;

    // 5) Unmask the bits for pins 0-3
    GPIO_PORTE_IM_R |= 0x0F;

    // 6) Enable GPIO port E interrupt
    NVIC_EN0_R |= 0x00000010;

    // Bind the interrupt to the handler.
    IntRegister(INT_GPIOE, gpioe_handler);
}


/**
 * Interrupt handler -- executes when a GPIO PortE hardware event occurs (i.e., for this lab a button is pressed)
 */
void gpioe_handler() {
    button_event = 1;
    button_num = button_getButton();

    // Clear interrupt status register
    GPIO_PORTE_ICR_R = 0xFF;
}




/**
 * Returns the position of the rightmost button being pushed.
 * @return the position of the rightmost button being pushed. 4 is the rightmost button, 1 is the leftmost button.  0 indicates no button being pressed
 */
uint8_t button_getButton() {

	// TODO: Write code below -- Return the rightmost button position pressed
	int res;
	uint32_t data = GPIO_PORTE_DATA_R;

	// INSERT CODE HERE!
    if (!(data & 0x0008) && !prev_b4) {
        res = 4;
    } else if (!(data & 0x0004) && !prev_b3) {
        res = 3;
    } else if (!(data & 0x0002) && !prev_b2) {
        res = 2;
    } else if (!(data & 0x0001) && !prev_b1) {
        res = 1;
    } else {
        res = 0;
    }

    prev_b4 = !(data & 0x0008);
    prev_b3 = !(data & 0x0004);
    prev_b2 = !(data & 0x0002);
    prev_b1 = !(data & 0x0001);

    return res;
}





