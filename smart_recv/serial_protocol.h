#define HEADER_SIZE 3

typedef enum MainHeader {
    GET = 0x00,
    SET = 0x01,
    UPGRADE = 0x02,
    RECEIVED_BEACON = 0x03,
    NAK = 0xFE,
    ACK = 0xFF
} MainHeader_t;

//Used for dealing with GET/SET
typedef enum ValueHeaders {
    FIRMWARE_VERSION = 0x00,
    HARDWARE_VERSION = 0x01,
    SERIAL_NUMBER = 0x02,
    REPORTER_ENABLED = 0x03
} ValueHeader_t;

