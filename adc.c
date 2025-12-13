/*
 *  Created on: Oct 20, 2025
 *      Author: gthomp
 */

#include "adc.h"
#include "putty.h"

#include <stdint.h>
#include <stdbool.h>
#include <inc/tm4c123gh6pm.h>
#include "driverlib/interrupt.h"

volatile char adc_flag = 0;
volatile short adc_data = 0x000;

// Triggers on ADC0 finish conversion, sets flag so program knows new IR data is ready
void IR_interrupt_handler() {
    if (ADC0_ISC_R & 0x02) {
		// write data from ADC0_SSFIFO1_R to somewhere
	    adc_data = (ADC0_SSFIFO1_R & 0xFFF);
	    adc_flag = 1;
	    ADC0_ISC_R |= 0x02;
	} else {
		return;
	}
}

// Initialize ADC0 SS1 on Port B with 64x hardware scans + interrupt
void adc_init() {
	SYSCTL_RCGCGPIO_R |= 0x2; // portb
    SYSCTL_RCGCADC_R |= 0x1; // adc0

	GPIO_PORTB_AFSEL_R |= 0x10; 
	GPIO_PORTB_DIR_R &= ~0x10;
	GPIO_PORTB_DEN_R &= ~0x10;
	GPIO_PORTB_AMSEL_R |= 0x10;
	GPIO_PORTB_ADCCTL_R &= 0x00;
	
	ADC0_ACTSS_R &= ~0x02; // clears SS1
	ADC0_EMUX_R &= ~0x00F0; // clears software triggers
	ADC0_SSMUX1_R &= 0xFFFFFFF0;
	ADC0_SSMUX1_R |= 0x0A; // sets mux0 for channel 10
	ADC0_SSCTL1_R &= 0xFFF0;
	ADC0_SSCTL1_R |= 0x06; // Sets to use ADCSSMUX, 1st sample raw interrupt enabled, and 1st sample is end of seq.
	ADC0_IM_R |= 0x2;
	ADC0_SAC_R &= ~0xF;
	ADC0_SAC_R |= 0x6; // 64x hardware sampling
	ADC0_ACTSS_R |= 0x02; // reenable SS1

	NVIC_EN0_R |= 0x8000;
	IntRegister(INT_ADC0SS1, IR_interrupt_handler);
	
	return;
}

// Start an ADC conversion for the IR
void adc_read() {
    ADC0_PSSI_R |= 0x2;
}
