#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "boards.h"
#include "nrf_gpio.h"
#include "serial.h"
#include "radio.h"

#define HUB_PACKET_MAX_SIZE 32
#define PACKET_S1_FIELD_SIZE             2  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE             1  /**< Packet S0 field size in bytes. */
#define PACKET_LENGTH_FIELD_SIZE         6  /**< Packet length field size in bits. */
#define PACKET_BASE_ADDRESS_LENGTH       3  //!< Packet base address length field size in bytes - 1

uint8_t sniffer_rx[255];
uint8_t sniffer_channel = 24;
unsigned received_count = 0;

void handlePacket()
{
    //uint8_t * packet = sniffer_rx;
}

void radio_init()
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
    NRF_RADIO->FREQUENCY = sniffer_channel;
    NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) | 
                        (RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos); 
    NRF_RADIO->PACKETPTR = (uint32_t)sniffer_rx;
    NRF_RADIO->INTENSET = (RADIO_INTENSET_END_Set << RADIO_INTENSET_END_Pos) | (RADIO_INTENSET_RSSIEND_Set << RADIO_INTENSET_RSSIEND_Pos); 
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_RXEN = 1;
    NRF_RADIO->EVENTS_RSSIEND = 0;
    NVIC_EnableIRQ(RADIO_IRQn);
}

void RADIO_IRQHandler(void) 
{
    if((NRF_RADIO->EVENTS_END) && 
      (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)) {
        NRF_RADIO->EVENTS_END = 0;
        if(NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle) {
            received_count++;
            nrf_gpio_pin_toggle(LED_1);
            unsigned char buffer[256];
            char rssi = NRF_RADIO->RSSISAMPLE;
            char crcError = NRF_RADIO->CRCSTATUS;
            if(crcError == 0) {
                sprintf(buffer, "CRC ERROR\n\r");
            }
            else {
                int length = sprintf(buffer, "-%ddBm|%02x|", rssi, sniffer_rx[0]);
                int msgLength = sniffer_rx[1];
                for(int i = 0; i < msgLength; i++) {
                    sprintf(&buffer[length + i*3], "%02x ", sniffer_rx[3 + i]);
                }
                strcat(buffer,"\r\n");
            }
            send_array(buffer, strlen(buffer));
            NRF_RADIO->TASKS_START = 1;
        }
    }
    if((NRF_RADIO->EVENTS_RSSIEND) &&
        (NRF_RADIO->INTENSET & RADIO_INTENSET_RSSIEND_Msk)) {
         NRF_RADIO->EVENTS_RSSIEND = 0;
    }
}
