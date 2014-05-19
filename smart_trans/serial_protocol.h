#define HEADER_SIZE 3

typedef enum MainHeader {
    GET = 0x00,
    SET = 0x01,
    UPGRADE = 0x02,
    NAK = 0xFE,
    ACK = 0xFF
} MainHeader_t;

//Used for dealing with GET/SET
typedef enum ValueHeaders {
    FIRMWARE_VERSION = 0x00,
    HARDWARE_VERSION = 0x01,
    SERIAL_NUMBER = 0x02,
    PERIOD = 0x03,
    OUTPUT_POWER = 0x04,
    PAYLOAD = 0x05
} ValueHeader_t;

typedef enum TransmitterOutputPower {
    OUTPUT_POWER_0dB,
    OUTPUT_POWER_Neg4dB,
    OUTPUT_POWER_Neg8dB,
    OUTPUT_POWER_Neg12dB,
    OUTPUT_POWER_Neg16dB,
    OUTPUT_POWER_Neg20dB,
    OUTPUT_POWER_Neg30dB,
} TransmitterOutputPower_t;
