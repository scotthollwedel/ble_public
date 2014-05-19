#define HEADER_SIZE 3

typedef enum MainHeader {
    GET = 0x00,
    SET = 0x01,
    UPGRADE = 0x02,
    NODE_JOIN = 0x03,
    NODE_ALERT = 0x04,
    GET_NODE_VALUE = 0x05,
    SET_NODE_VALUE = 0x06,
    UPGRADE_NODE = 0x07,
    NAK = 0xFE,
    ACK = 0xFF
} MainHeader_t;

//Used for dealing with GET/SET
typedef enum ValueHeaders {
    FIRMWARE_VERSION = 0x00,
    HARDWARE_VERSION = 0x01,
    SERIAL_NUMBER = 0x02,
    HUB_ENABLED = 0x03,
    HUB_JOINABLE = 0x04,
} ValueHeader_t;
