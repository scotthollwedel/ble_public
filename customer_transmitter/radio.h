#ifndef RADIO_H
#define RADIO_H
#include <stdint.h>
void radio_init(void);
void sendPacket(const uint8_t power, const uint8_t frequency, const uint8_t * packet);
#endif
