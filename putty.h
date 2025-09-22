/*
 * putty.h
 *
 *  Created on: Sep 19, 2025
 *      Author: seankru
 */

#ifndef PUTTY_H_
#define PUTTY_H_

#include "cyBot_Scan.h"
#include "sensor.h"

void putty_send_object(Object_t *object);
void putty_send_scan_data(int angle, cyBOT_Scan_t *scan_data);

// Takes a char array and sends its content one byte at a time over UART to PuTTy
// cyBot_uart_init *must* have been previously called
void putty_send_string(char *str, int length);


// String MUST be null terminated and less than 50 characters
// cyBot_uart_init *must* have been previously called
void putty_printf(const char *format, ...);

#endif /* PUTTY_H_ */
