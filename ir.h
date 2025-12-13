/*
 * ir.h
 *
 *  Created on: Nov 16, 2025
 *      Author: seankru
 */

#ifndef IR_H_
#define IR_H_

// Configures the IR sensor on Port B Pin 2
void configure_reciever();

// Reads the current value from the IR
int read_reciever();

#endif /* IR_H_ */
