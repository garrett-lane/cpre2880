
#include "ir.h"

#include <stdint.h>
#include <inc/tm4c123gh6pm.h>

void configure_reciever() {
    SYSCTL_RCGCGPIO_R |= 0x02;      // enable clock GPIOB (page 340)
    GPIO_PORTB_DIR_R   &= ~0x04;         // pb2 as input
    GPIO_PORTB_AFSEL_R &= ~0x04;      // Disable PB2 alternate select (page 671)
    GPIO_PORTB_PCTL_R  &= ~0xF00;       // Clear PB2 pctl (page 688)  also refer to page 650
    GPIO_PORTB_DEN_R   |= 0x04;        // enables pb2
}


int read_reciever() {
    return (GPIO_PORTB_DATA_R & 0x4) >> 2;
}
