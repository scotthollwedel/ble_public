#include <stdint.h>
#include <string.h>
#include "ota_protocol.h"
#include "nrf_gpio.h"
#include "radio.h"

#define HUB_PACKET_MAX_SIZE 32
#define PACKET_S1_FIELD_SIZE             2  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE             1  /**< Packet S0 field size in bytes. */
#define PACKET_LENGTH_FIELD_SIZE         6  /**< Packet length field size in bits. */
#define PACKET_BASE_ADDRESS_LENGTH       3  //!< Packet base address length field size in bytes - 1

uint8_t hub_tx[255];
uint8_t hub_rx[255];
uint8_t hub_channel = 24;

uint8_t packetQueueHead = 0;
uint8_t packetQueueTail = 0;
uint8_t packetQueue[16][HUB_PACKET_MAX_SIZE];
uint8_t pilot[HUB_PACKET_MAX_SIZE];

struct Nodes
{
    uint8_t nodeAddress[6];
};
struct Nodes addressTable[8];

uint8_t nodeCount = 0;

void hub_init()
{
    NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit;
    NRF_RADIO->BASE0       = 0x89BED600;  // Base address for prefix 0 converted to nRF24L series format
    NRF_RADIO->PREFIX0     = (0x8E << RADIO_RXADDRESSES_ADDR0_Pos);
    NRF_RADIO->TXADDRESS   = 0x00UL;      // Set device address 0 to use when transmitting
    NRF_RADIO->RXADDRESSES = 0x01UL;
    NRF_RADIO->PCNF0 = (PACKET_S1_FIELD_SIZE << RADIO_PCNF0_S1LEN_Pos) |
                        (PACKET_S0_FIELD_SIZE << RADIO_PCNF0_S0LEN_Pos) |     
                     (PACKET_LENGTH_FIELD_SIZE << RADIO_PCNF0_LFLEN_Pos);
    NRF_RADIO->PCNF1 = (HUB_PACKET_MAX_SIZE << RADIO_PCNF1_MAXLEN_Pos) | 
                     (PACKET_BASE_ADDRESS_LENGTH << RADIO_PCNF1_BALEN_Pos);
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos) | 
                      (RADIO_CRCCNF_SKIP_ADDR_Skip << RADIO_CRCCNF_SKIP_ADDR_Pos); //0x0103;
    NRF_RADIO->CRCINIT = 0x555555;
    NRF_RADIO->CRCPOLY = 0x065B;  
}

void setPilot() 
{
    pilot[0] = 0x00;//ADV_IND w/ random S0
    pilot[1] = sizeof(struct PilotMessage); //Payload size
    pilot[2] = 0x00;//Empty field for S1
    struct PilotMessage * pilotMessage = (struct PilotMessage *)&pilot[3];
    pilotMessage->messageType = PILOT;
    pilotMessage->messageId = 0;
    pilotMessage->protocolNumber = 0;
    pilotMessage->sync = 1;
    //Base Address
    for(int i = 0; i < 4; i++)
        pilotMessage->sourceAddress[i] = NRF_FICR->DEVICEADDR[0] >> (i*8);
    for(int i = 0; i < 2 ; i++)
        pilotMessage->sourceAddress[4 + i] = NRF_FICR->DEVICEADDR[1] >> (i*8);
}

void handlePacket()
{
    uint8_t * packet = hub_rx;
    MessageType_t messageType = (MessageType_t)packet[3];
    if(messageType == JOIN) {
        struct JoinMessage * joinMessage = (struct JoinMessage *)&packet[3];
        int found = 0;
        for(int i = 0; i < nodeCount && found == 0; i++) {
            if(memcmp(addressTable[i].nodeAddress, joinMessage->sourceAddress, 6) == 0)
            {
                if(packetQueueTail + 1 != packetQueueHead) {
                    uint8_t * newPacket = packetQueue[packetQueueTail];
                    packetQueueTail++;
                    packetQueueTail |= 16 - 1;
                    
                    newPacket[0] =  0x00;
                    newPacket[1] = sizeof(struct JoinAckMessage);
                    newPacket[2] = 0x00;
                    
                    struct JoinAckMessage * joinAckMessage = (struct JoinAckMessage *)&newPacket[3];
                    joinAckMessage->messageType = JOIN_ACK;
                    joinAckMessage->messageId = 0;
                    memmove(joinAckMessage->destinationAddress, joinMessage->sourceAddress, 6);
                    memmove(joinAckMessage->sourceAddress, joinMessage->destinationAddress, 6);
                    joinAckMessage->assignedNodeId = i;
                    found = 1;
                }
            }
        }
        if(0 == found)
        {
            if(packetQueueTail + 1 != packetQueueHead) {
                uint8_t * newPacket = packetQueue[packetQueueTail];
                packetQueueTail++;
                packetQueueTail |= 16 - 1;
                
                newPacket[0] =  0x00;
                newPacket[1] = sizeof(struct JoinAckMessage);
                newPacket[2] = 0x00;
                
                struct JoinAckMessage * joinAckMessage = (struct JoinAckMessage *)&newPacket[3];
                joinAckMessage->messageType = JOIN_ACK;
                joinAckMessage->messageId = 0;
                memmove(joinAckMessage->destinationAddress, joinMessage->sourceAddress, 6);
                memmove(joinAckMessage->sourceAddress, joinMessage->destinationAddress, 6);
                joinAckMessage->assignedNodeId = nodeCount;
            }
            nodeCount++;
        }
    }
    else if (messageType == GET_VALUE_RESPONSE) {
    }
    else if(messageType == RESPONSE) {
    }
}
