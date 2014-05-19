#ifndef _BLE_H
#define _BLE_H
#define TICKS_BETWEEN_BLE_TRANSMIT .000150/HF_CLOCK_PERIOD

void ble_init(void);
int getBLERFChannel(int index);
int getBLELogicalChannel(int index);
void sendBLEBeacon(int index);
#endif
