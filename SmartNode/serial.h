#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
char serialBuffer[1024];
void uart_init(void);
void debug_print(const char * str);
#endif
