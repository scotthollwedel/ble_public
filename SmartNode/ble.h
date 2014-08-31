#ifndef _BLE_H
#define _BLE_H
#include <stdint.h>

void ble_init(void);
int getBLERFChannel(int index);
int getBLELogicalChannel(int index);
void setBLEBeaconPayload(const uint8_t * beaconPayload, const unsigned int beaconPayloadSize);
void setBLEBeaconTransmitPeriod(const uint8_t transmitPeriod);
int getBLEBeaconTransmitPeriod();
void setBLEBeaconTransmitPower(const int8_t transmitPower);
int getBLEBeaconTransmitPower();
void sendBLEBeacon(int index);
#endif
