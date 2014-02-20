#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_delay.h"

#define PACKET_BASE_ADDRESS_LENGTH       (3UL)  //!< Packet base address length field size in bytes
#define PACKET_STATIC_LENGTH             (37UL)  //!< Packet static length in bytes
#define PACKET_PAYLOAD_MAXSIZE           (PACKET_STATIC_LENGTH)  //!< Packet payload maximum size in bytes

static uint8_t packet[PACKET_PAYLOAD_MAXSIZE];  /**< Packet to transmit. */
static uint8_t address[] = {0xA8, 0x11, 0x1A, 0x6D, 0xC4, 0xCE}; //6 Bytes
static uint8_t advinfo[] = {2,1,4,0x1A, 0xFF}; //5 Bytes
static uint8_t mfginfo[] = {0x4C, 0x00, 0x02, 0x15, 0x01, 
	                          0x12, 0x23, 0x34, 0x45, 0x56,
	                          0x67, 0x78, 0x89, 0x9A, 0xAB,
	                          0xBC, 0xCD, 0xDE, 0xEF, 0xF0,
	                          0x00, 0x00, 0x00, 0x01, 0xC3};

/* These are set to zero as Shockburst packets don't have corresponding fields. */
#define PACKET_S1_FIELD_SIZE             2  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE             1  /**< Packet S0 field size in bytes. */
#define PACKET_LENGTH_FIELD_SIZE         6  /**< Packet length field size in bits. */

void radio_configure()
{
    // Radio config
    NRF_RADIO->MODE      = 3UL;
	  NRF_RADIO->TXPOWER   = 0UL;

    // Radio address config
    NRF_RADIO->BASE0       = 0x89BED600UL;  // Base address for prefix 0 converted to nRF24L series format
	  NRF_RADIO->PREFIX0     = 0x0000008EUL;
	
	  NRF_RADIO->TXADDRESS   = 0x00UL;      // Set device address 0 to use when transmitting
    NRF_RADIO->RXADDRESSES = 0x01UL;    // Enable device address 0 to use to select which addresses to receive

    NRF_RADIO->PCNF0 = 0x00020106UL;   //S1LEN = 2 bits, S0LEN = 1 byte, LFLEN=6 bits
    NRF_RADIO->PCNF1 = 0x02030025UL;   //MAXLEN = 37, BALEN=3, WHITEEN=True
    NRF_RADIO->CRCCNF = 0x0103;
	  NRF_RADIO->CRCINIT = 0x555555;
	  NRF_RADIO->CRCPOLY = 0x065B;
}

int main(void) 
{
		nrf_gpio_range_cfg_output(LED_START, LED_STOP);
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    // Wait for the external oscillator to start up.
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) 
    {
        // Do nothing.
    }
		
		radio_configure();
		
		nrf_gpio_pin_set(LED_0);
		
		uint8_t channels[] = {2, 26, 80};
		uint8_t blechannels[] = {37, 38, 39};
		packet[0] = 0x42;//ADV_IND w/ random S0
		packet[1] = 0x24;//Size 36 bytes LENGTH
		packet[2] = 0x00;//Empty field for S1
		memcpy(&packet[3], address, sizeof(address));
		memcpy(&packet[3 + sizeof(address)], advinfo, sizeof(advinfo));
		memcpy(&packet[3 + sizeof(address) + sizeof(advinfo)], mfginfo, sizeof(mfginfo));
		NRF_RADIO->PACKETPTR    = (uint32_t)packet;
    while(true) {
				for(int j = 0; j < sizeof(channels); j++)
				{
					NRF_RADIO->FREQUENCY = channels[j];          
					NRF_RADIO->DATAWHITEIV = blechannels[j] + 0x40;
					NRF_RADIO->EVENTS_READY = 0U;
					NRF_RADIO->TASKS_TXEN   = 1; // Enable radio and wait for ready.

					while (NRF_RADIO->EVENTS_READY == 0U)
					{
					}

					// Start transmission.
					NRF_RADIO->TASKS_START = 1U;
					NRF_RADIO->EVENTS_END  = 0U;
			
					while(NRF_RADIO->EVENTS_END == 0U) // Wait for end of the transmission packet.
					{
					}
					NRF_RADIO->EVENTS_DISABLED = 0U;
					NRF_RADIO->TASKS_DISABLE   = 1U; // Disable the radio.

					while(NRF_RADIO->EVENTS_DISABLED == 0U)
					{
					}
					nrf_delay_ms(10);
				}
				nrf_gpio_pin_toggle(LED_1);
				nrf_delay_ms(500);
		}
}

