#ifndef STATE_VARIABLES_H
#define STATE_VARIABLES_H
#include <stdint.h>

#define LF_CLOCK_PERIOD         		 32768
#define HF_CLOCK_PERIOD                  16000000
#define LOOP_PERIOD_IN_MS                100

typedef enum SystemMode {
    IDLE_MODE,
	TRANSMITTER_MODE,
	RECEIVER_MODE,
	HUB_MODE
} SystemMode_t;

void init_state_variables(void);
void setNewMode(const SystemMode_t newMode);
SystemMode_t getNewMode(void);
void setMode(const SystemMode_t mode);
SystemMode_t getMode(void);
void setBLEBeaconPayload(const uint8_t * beaconPayload, const unsigned int beaconPayloadSize);
void getBLEBeaconPayload(const uint8_t * beaconPayload, const unsigned int * beaconPayloadSize);
void setBLEBeaconTransmitPeriod(const uint8_t transmitPeriod);
int getBLEBeaconTransmitPeriod(void);
void setBLEBeaconTransmitPower(const uint8_t transmitPower);
int getBLEBeaconTransmitPower(void);
#endif
