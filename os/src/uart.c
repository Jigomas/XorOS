#include "uart.h"

void uart_putchar(char c) {
    *(volatile uint32_t*) UART_TX_ADDR = (uint32_t) (unsigned char) c;
}

void uart_puts(const char* s) {
    while (*s)
        uart_putchar(*s++);
}
