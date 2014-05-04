#include <stdint.h>

typedef enum MessageType
{
    PILOT = 0x00,
    JOIN = 0x01,
    JOIN_ACK = 0x02,
    SET_VALUE = 0x03,
    GET_VALUE = 0x04,
    GET_VALUE_RESPONSE = 0x05,
    RESPONSE = 0xFF
    
} MessageType_t;

__packed struct PilotMessage
{
    uint8_t messageType;
    uint8_t messageId;
    uint8_t sourceAddress[6];
    uint8_t protocolNumber;
    uint8_t sync;
};

__packed struct JoinMessage
{
    uint8_t messageType;
    uint8_t messageId;
    uint8_t destinationAddress[6];
    uint8_t sourceAddress[6];
    uint8_t protocolNumber;
    uint8_t firmwareVersion[3];
};

__packed struct JoinAckMessage
{
    uint8_t messageType;
    uint8_t messageId;
    uint8_t sourceAddress[6];
    uint8_t destinationAddress[6];
    uint8_t assignedNodeId;
};

__packed struct SetValueMessage
{
    uint8_t messageType;
    uint8_t messageId;
    uint8_t sourceAddress[6];
    uint8_t sourceNodeId;
    uint8_t valueType;
    uint8_t value;
};

__packed struct GetValueMessage
{
    uint8_t messageType;
    uint8_t messageId;
    uint8_t sourceAddress[6];
    uint8_t sourceNodeId;
    uint8_t valueType;
};

__packed struct GetValueResponseMessage
{
    uint8_t messageType;
    uint8_t responseMessageId;
    uint8_t destinationAddress[6];
    uint8_t sourceNodeId;
    uint8_t value;
};

__packed struct ResponseMessage
{
    uint8_t messageType;
    uint8_t responseMessageId;
    uint8_t destinationAddress[6];
    uint8_t sourceNodeId;
};
