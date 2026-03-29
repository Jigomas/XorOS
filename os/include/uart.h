#ifndef UART_H
#define UART_H

#include <stdint.h>

// MMIO UART: TX register at 0xF000
// write byte → simulator prints char to stdout
#define UART_TX_ADDR 0xF000u

void uart_putchar(char c);
void uart_puts(const char* s);

#endif /* UART_H */
