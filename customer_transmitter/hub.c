#include <stdint.h>
#include "nrf_gpio.h"
#include "radio.h"

#define HUB_PACKET_MAX_SIZE 32
#define PACKET_S1_FIELD_SIZE             2  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE             1  /**< Packet S0 field size in bytes. */
#define PACKET_LENGTH_FIELD_SIZE         6  /**< Packet length field size in bits. */
#define PACKET_BASE_ADDRESS_LENGTH       3  //!< Packet base address length field size in bytes - 1
#define PACKET_PAYLOAD_MAXSIZE           37

uint8_t hub_tx[255];
uint8_t hub_rx[255];
uint8_t hub_channel = 24;

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

void sendBeacon() 
{
    uint8_t packet[] = {0x00, 10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    hub_tx[0] = 0x00;//Source Node ID = 0??
    hub_tx[1] = 10; //Payload size
    hub_tx[2] = 0x00;//Empty field for S1
    hub_tx[3] = 0x00; //Beacon
    hub_tx[4] = 0xFF; //Broadcast message
    //Base Address
    for(int i = 0; i < 4; i++)
        packet[5 + i] = NRF_FICR->DEVICEADDR[0] >> (i*8);
    for(int i = 0; i < 2 ; i++)
        packet[9 + i] = NRF_FICR->DEVICEADDR[1] >> (i*8);
    hub_tx[11] = 0x00; // Protocol number
    hub_tx[12] = 0x00; //Sync
    sendPacket(RADIO_TXPOWER_TXPOWER_Pos4dBm, 24, packet);
}
