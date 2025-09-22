/*
 * putty.c
 *
 *  Created on: Sep 19, 2025
 *      Author: seankru
 */

#include "putty.h"

#include "cyBot_uart.h"

#define TOTAL_CHARS 50

void putty_send_object(Object_t *object) {
    putty_printf("OBJECT %f %f %d\n", object->center_angle, object->distance, object->linear_width);
}

void putty_send_scan_data(int angle, cyBOT_Scan_t *scan_data) {
    putty_printf("%3d\t\t%10f\n", angle, scan_data->sound_dist);
}

void putty_send_string(char *str, int length) {
    int i = 0;
    for (i = 0; i < length; i++) {
        cyBot_sendByte(str[i]);
    }
}

void putty_printf(const char *format, ...) {
    char buffer[TOTAL_CHARS + 1];
    va_list arglist;
    va_start(arglist, format);
    vsnprintf(buffer, TOTAL_CHARS + 1, format, arglist);
    buffer[TOTAL_CHARS] = '\0';

    char *str = buffer;
    while (*str) {
        if (*str == '\n') {
            cyBot_sendByte('\r');
            cyBot_sendByte(*str);
        } else {
            cyBot_sendByte(*str);
        }

        str++;
    }
    va_end(arglist);
}

