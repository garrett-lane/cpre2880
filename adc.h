/*
 *  Created on: Oct 20, 2025
 *      Author: gthomp
 */

#ifndef ADC_H_
#define ADC_H_

extern volatile short adc_data;
extern volatile char adc_flag;

// Initialize ADC0 SS1 on Port B with 64x hardware scans + interrupt
void adc_init(void);

// Start an ADC conversion for the IR
void adc_read(void);

#endif /* ADC_H_ */
