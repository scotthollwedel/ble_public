#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
void uart_init(void);
void send_array(uint8_t * array, uint16_t size);

void HandleTransmitter(unsigned char * buffer, uint8_t size);
#endif
