#include <stdint.h>
#include <string.h>
#include "nrf_gpio.h"
#include "state_variables.h"

//BLE Values
#define MAX_BLE_BEACON_SIZE 255
#define MIN_BLE_BEACON_SIZE 9
#define MAX_BLE_BEACON_PAYLOAD_SIZE MAX_BLE_BEACON_SIZE - MIN_BLE_BEACON_SIZE

struct SystemStateVariables {
    SystemMode_t newMode;
    SystemMode_t mode;
}systemStateVariables;

struct BLEStateVariables {
    uint8_t BeaconPayload[MAX_BLE_BEACON_PAYLOAD_SIZE];
    unsigned int BeaconPayloadSize;
    unsigned int TransmitPeriod;
    unsigned int TransmitPower;
} bleStateVariables;

void init_state_variables()
{
    //System State Variables
    systemStateVariables.newMode = HUB_MODE;
    systemStateVariables.mode = IDLE_MODE;
    //BLE State Variables
    memset(bleStateVariables.BeaconPayload, 0,MAX_BLE_BEACON_PAYLOAD_SIZE);
    bleStateVariables.BeaconPayloadSize = 0;
    bleStateVariables.TransmitPeriod = 10;
    bleStateVariables.TransmitPower = RADIO_TXPOWER_TXPOWER_0dBm;
}

void setNewMode(const SystemMode_t newMode)
{
    systemStateVariables.newMode = newMode;
}

SystemMode_t getNewMode()
{
    return systemStateVariables.newMode;
}

void setMode(const SystemMode_t currentMode)
{
    systemStateVariables.mode = currentMode;
}

SystemMode_t getMode()
{
    return systemStateVariables.mode;
}
void setBLEBeaconPayload(const uint8_t * beaconPayload, const unsigned int beaconPayloadSize)
{
    memcpy(bleStateVariables.BeaconPayload, beaconPayload, beaconPayloadSize);
    bleStateVariables.BeaconPayloadSize = beaconPayloadSize;
}

unsigned getBLEBeaconPayload(const uint8_t * beaconPayload)
{
    beaconPayload = bleStateVariables.BeaconPayload;
    return bleStateVariables.BeaconPayloadSize;
}

void setBLEBeaconTransmitPeriod(const uint8_t transmitPeriod)
{
    bleStateVariables.TransmitPeriod = transmitPeriod;
}

int getBLEBeaconTransmitPeriod ()
{
    return bleStateVariables.TransmitPeriod;
}

void setBLEBeaconTransmitPower(const uint8_t transmitPower)
{
    bleStateVariables.TransmitPower = transmitPower;
}

int getBLEBeaconTransmitPower()
{
    return bleStateVariables.TransmitPower;
}
