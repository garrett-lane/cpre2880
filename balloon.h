/*
 * balloon.h
 *
 *  Created on: Dec 2nd, 2025
 *      Author: seankru
 */

#ifndef BALLOON_H_
#define BALLOON_H_

#include <stdint.h>
#include <stdbool.h>
#include <inc/tm4c123gh6pm.h>
#include "driverlib/interrupt.h"

volatile int balloon_flag;

void balloon_init();
int read_balloon();
void balloon_handler();

#endif /* BALLOON_H_ */
