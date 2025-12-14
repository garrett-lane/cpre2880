/*
*   uart.c
        @author Garrett Thompson & Sean Krueger 10/7/2025
*/

#include "uart.h"

#include "timer.h"

volatile char uart_flag = 0;

void uart_init(int baud) {
    SYSCTL_RCGCGPIO_R |= 0x02;   // enable clock GPIOB (page 340)
    SYSCTL_RCGCUART_R |= 0x02;   // enable clock UART1 (page 344)
    GPIO_PORTB_AFSEL_R |= 0x03;  // sets PB0 and PB1 as peripherals (page 671)
    GPIO_PORTB_PCTL_R |=
        0x00000011;  // pmc0 and pmc1       (page 688)  also refer to page 650
    GPIO_PORTB_DEN_R |= 0x03;    // enables pb0 and pb1
    GPIO_PORTB_DIR_R |= 0x02;    // pb1 as output
    GPIO_PORTB_DIR_R &= 0xFFFE;  // sets pb0 as input

    // compute baud values [UART clock = 16 MHz]
    double fbrd;
    int ibrd;

    // BRD = 16,000,000 / (16 * 115200) = 8.6805
    // IBRD = 8
    // FBRD = floor(0.6805 * 64 + 0.5) = 44

    fbrd = 0b00101100;  // page 903
    ibrd = 0b00001000;

    UART1_CTL_R &= 0xFCFE;       // disable UART1 (page 918)
    UART1_IBRD_R = ibrd;         // write integer portion of BRD to IBRD
    UART1_FBRD_R = fbrd;         // write fractional portion of BRD to FBRD
    UART1_LCRH_R &= 0xFFFFFF00;  // write serial communication parameters (page
                                 // 916) * 8bit and no parity, no FIFO
    UART1_LCRH_R |= 0x6C;        // 0b01101100 0x6C
    UART1_CC_R = 0x0;            // use system clock as clock source (page 939)
    UART1_CTL_R |= 0x0301;       // enable UART1
}

// Blocking cal that sends 1 char over UART 1
void uart_sendChar(char data) {
    // wait here as long as the FIFO is full
    while (UART1_FR_R & 0x20) {
        // wait like a good boy
    }

    // Send data
    UART1_DR_R = data;
}

char uart_receive(void) {
    char data = 0;
    // keep waiting as long as FIFO is empty
    while (UART1_FR_R & 0x10) {
    }

    // mask the 4 error bits and grab only 8 data bits
    data = (char)(UART1_DR_R & 0xFF);
    return data;
}

void uart_sendStr(const char *data) {
    while (*data) {
        uart_sendChar(*data);
        data++;
    }
}

// _PART3

void uart_interrupt_init() {
    // Enable interrupts for receiving bytes through UART1
    UART1_IM_R |= 0x0010;  // enable interrupt on receive - page 924

    // Find the NVIC enable register and bit responsible for UART1 in table 2-9
    // Note: NVIC register descriptions are found in chapter 3.4
    NVIC_EN0_R |= 0x0040;  // enable uart1 interrupts - page 104

    // Find the vector number of UART1 in table 2-9 ! UART1 is 22 from vector
    // number page 104
    IntRegister(INT_UART1,
                uart_interrupt_handler);  // give the microcontroller the
                                          // address of our interrupt handler -
                                          // page 104 22 is the vector number
}

void uart_interrupt_handler() {
    // STEP1: Check the Masked Interrup Status
    if (uart_flag) {
        return;
    }

    if (UART1_MIS_R & 0x0010) {
        // STEP2:  Copy the data
        uart_flag = 1;
        uart_data = (UART1_DR_R & 0x000000FF);
        // STEP3:  Clear the interrupt
        UART1_ICR_R |= 0x00000010;
    }
}
