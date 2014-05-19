#define HEADER_SIZE 3

typedef enum MainHeader {
	CHANGE_MODE,
	GET_FIRMWARE_VERSION,
	GET_HARDWARE_VERSION,
	GET_SERIAL_NUMBER,
	GET_BATTERY,
	TRANSMITTER,
	RECEIVER,
	HUB,
	LOCAL_UPGRADE,
	NAK = 0xFE,
	ACK = 0xFF
} MainHeader_t;

typedef enum ChangeMode {
	CHANGE_MODE_TRANSMITTER,
	CHANGE_MODE_RECEIVER,
	CHANGE_MODE_HUB
} ChangeMode_t;

typedef enum Transmitter {
	SET_TRANSMITTER_VALUE,
	GET_TRANSMITTER_VALUE
} Transmitter_t;

typedef enum TransmitterValues {
	TRANSMITTER_PERIOD,
	TRANSMITTER_OUTPUT_POWER,
	TRANSMITTER_PAYLOAD,
} TransmitterValues_t;

typedef enum TransmitterOutputPower {
    OUTPUT_POWER_0dB,
    OUTPUT_POWER_Neg4dB,
    OUTPUT_POWER_Neg8dB,
    OUTPUT_POWER_Neg12dB,
    OUTPUT_POWER_Neg16dB,
    OUTPUT_POWER_Neg20dB,
    OUTPUT_POWER_Neg30dB,
} TransmitterOutputPower_t;
