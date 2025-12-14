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

// Send a Object_t over UART
void putty_send_object(Object_t *object);

// Send a scan interval over UART
void putty_send_scan_data(int angle, cyBOT_Scan_t *scan_data);

// Send the bot's current position over UART
void putty_send_pos();

// Send the string over UART
void putty_puts(char string[], int length);

// String MUST be null terminated and less than 50 characters
// cyBot_uart_init *must* have been previously called
void putty_printf(const char *format, ...);

#endif /* PUTTY_H_ */
