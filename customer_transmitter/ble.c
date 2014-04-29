#include <stdint.h>
#include <string.h>
#include "nrf_gpio.h"
#include "time.h"
#include "ble.h"
#include "state_variables.h"

#define PACKET_S1_FIELD_SIZE             2  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE             1  /**< Packet S0 field size in bytes. */
#define PACKET_LENGTH_FIELD_SIZE         6  /**< Packet length field size in bits. */
#define PACKET_BASE_ADDRESS_LENGTH       3  //!< Packet base address length field size in bytes - 1
#define PACKET_PAYLOAD_MAXSIZE           37

void ble_init()
{
    NRF_RADIO->MODE      = RADIO_MODE_MODE_Ble_1Mbit;
    NRF_RADIO->BASE0       = 0x89BED600;  // Base address for prefix 0 converted to nRF24L series format
    NRF_RADIO->PREFIX0     = (0x8E << RADIO_RXADDRESSES_ADDR0_Pos);
    NRF_RADIO->TXADDRESS   = 0x00UL;      // Set device address 0 to use when transmitting
    NRF_RADIO->RXADDRESSES = 0x01UL;    // Enable device address 0 to use to select which addresses to receive
    NRF_RADIO->PCNF0 = (PACKET_S1_FIELD_SIZE << RADIO_PCNF0_S1LEN_Pos) | 
                     (PACKET_S0_FIELD_SIZE << RADIO_PCNF0_S0LEN_Pos) | 
                     (PACKET_LENGTH_FIELD_SIZE << RADIO_PCNF0_LFLEN_Pos);
    NRF_RADIO->PCNF1 = (PACKET_PAYLOAD_MAXSIZE << RADIO_PCNF1_MAXLEN_Pos) | 
                     (PACKET_BASE_ADDRESS_LENGTH << RADIO_PCNF1_BALEN_Pos) | 
                     (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos) | 
                      (RADIO_CRCCNF_SKIP_ADDR_Skip << RADIO_CRCCNF_SKIP_ADDR_Pos); //0x0103;
    NRF_RADIO->CRCINIT = 0x555555;
    NRF_RADIO->CRCPOLY = 0x065B;  
}

void sendBLEBeacon(int index)
{
    uint8_t * beacon_payload;
    unsigned int beacon_payload_size;
    uint8_t packet[255];
    
    getBLEBeaconPayload(beacon_payload, &beacon_payload_size);
    
    packet[0] = 0x40;//ADV_IND w/ random S0
    packet[1] = 6 + beacon_payload_size; //Payload size
    packet[2] = 0x00;//Empty field for S1
    
    for(int i = 0; i < 4; i++)
        packet[3 + i] = NRF_FICR->DEVICEADDR[0] >> (i*8);
    for(int i = 0; i < 2 ; i++)
        packet[7 + i] = NRF_FICR->DEVICEADDR[1] >> (i*8);

    memcpy(&packet[9], beacon_payload, beacon_payload_size);
    NRF_RADIO->TXPOWER   = getBLEBeaconTransmitPower();
    NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
                        (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos);
    NRF_RADIO->FREQUENCY = getBLERFChannel(index);          
    NRF_RADIO->DATAWHITEIV = getBLELogicalChannel(index) + 0x40;
    NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN   = 1;
}

int getBLERFChannel(int index)
{
    uint8_t channels[] = {2, 26, 80};
    return channels[index];
}

int getBLELogicalChannel(int index)
{
    uint8_t blechannels[] = {37, 38, 39};
    return blechannels[index];
    
}
