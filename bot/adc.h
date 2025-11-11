/*
 *  Created on: Oct 20, 2025
 *      Author: gthomp
 */

#ifndef ADC_H_
#define ADC_H_

extern volatile  short adc_data;
extern volatile  char adc_flag;

void adc_init(void);
void adc_read(void);
void IR_interrupt_handler();

#endif /* ADC_H_ */
