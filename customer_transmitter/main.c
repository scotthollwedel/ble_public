#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_delay.h"
#include "serial_protocol.h"
#include "version.h"

//BLE Information
static bool timerEnabled = false;
static bool listener = false;
static bool transmitter = false;
//static struct ble_nodes nodes[256];
static uint8_t recv_packet[37];
static uint8_t advinfo[] = {0x02,0x01,0x1A,0x1A,0xFF,0x4C,0x00,0x02,0x15}; //Apples Fixed iBeacon advertising prefix
static uint8_t mfginfo[] = {0x3A, 0xB0, 0x2B, 0xB7, 0x6A, 0xEC, 0x47, 0xBA, 0xAF,0x56, 0xAB, 0x0B, 0x2A, 0x9E, 0x0A, 0xA0}; //Proximity UUID 
static uint8_t major[] = {0x00, 0x01}; 
static uint8_t minor[] = {0x00, 0x01};
static uint8_t txPower = 0xC5;

/* UART RX BUFFER HANDLER */
static uint8_t uartRxBuffer[100];
static uint8_t rxBufferSize = 0;

#define PACKET_S1_FIELD_SIZE             2  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE             1  /**< Packet S0 field size in bytes. */
#define PACKET_LENGTH_FIELD_SIZE         6  /**< Packet length field size in bits. */
#define PACKET_BASE_ADDRESS_LENGTH       3  //!< Packet base address length field size in bytes - 1
#define PACKET_PAYLOAD_MAXSIZE           37
#define DEFAULT_SLEEP_TIME_IN_MS         100 //Max time is 2 seconds

void radio_init()
{
    // Radio config
    NRF_RADIO->MODE      = RADIO_MODE_MODE_Ble_1Mbit;
	  NRF_RADIO->TXPOWER   = RADIO_TXPOWER_TXPOWER_0dBm;

    // Radio address config
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
		
		NRF_RADIO->INTENSET = (RADIO_INTENSET_DISABLED_Enabled << RADIO_INTENSET_DISABLED_Pos) | 
					                (RADIO_INTENSET_END_Enabled << RADIO_INTENSET_END_Pos);
		NVIC_EnableIRQ(RADIO_IRQn);
}

//Configuration TIMER1 with a 31250 us period
void timer1_init()
{
		NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;
		NRF_TIMER1->PRESCALER = 9; //16000000/2^9 = 31250 Ticks/Sec
		NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_16Bit; //Roll over at 2^16 = 65536. Max bitmode is 16 bit.
		NRF_TIMER1->SHORTS = (TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos);
		NRF_TIMER1->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
		NVIC_EnableIRQ(TIMER1_IRQn);
}

void hfc_init()
{
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;
	  // Wait for the external oscillator to start up. Required to use radio.
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

void gpio_init(void)
{
		nrf_gpio_range_cfg_output(LED_START, LED_STOP);
    nrf_gpio_cfg_input(BUTTON_0, BUTTON_PULL);
    // Enable interrupt:
    NVIC_EnableIRQ(GPIOTE_IRQn);
    NRF_GPIOTE->CONFIG[0] =  (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos)
                           | (0 << GPIOTE_CONFIG_PSEL_Pos)  
                           | (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos);
	  NRF_GPIOTE->CONFIG[1] =  (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos)
                           | (1 << GPIOTE_CONFIG_PSEL_Pos)  
                           | (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos);
    NRF_GPIOTE->INTENSET  = (GPIOTE_INTENSET_IN0_Set << GPIOTE_INTENSET_IN0_Pos) 
	                         | (GPIOTE_INTENSET_IN1_Set << GPIOTE_INTENSET_IN1_Pos);
}

void uart_init(void)
{
  nrf_gpio_cfg_output(TX_PIN_NUMBER);
  nrf_gpio_cfg_input(RX_PIN_NUMBER, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_output(RTS_PIN_NUMBER);
  nrf_gpio_cfg_input(CTS_PIN_NUMBER, NRF_GPIO_PIN_NOPULL);  
  NRF_UART0->PSELTXD          = TX_PIN_NUMBER;
  NRF_UART0->PSELRXD          = RX_PIN_NUMBER;
	NRF_UART0->PSELCTS          = CTS_PIN_NUMBER;
  NRF_UART0->PSELRTS          = RTS_PIN_NUMBER;
	NRF_UART0->CONFIG           = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
  NRF_UART0->BAUDRATE         = (UART_BAUDRATE_BAUDRATE_Baud38400 << UART_BAUDRATE_BAUDRATE_Pos);
  NRF_UART0->ENABLE           = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);

	NVIC_EnableIRQ(UART0_IRQn);
	NRF_UART0->INTENSET					=  (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos);
  NRF_UART0->TASKS_STARTTX    = 1;
  NRF_UART0->TASKS_STARTRX    = 1;
  NRF_UART0->EVENTS_RXDRDY    = 0;
}

void send_byte(char c)
{
	NRF_UART0->TXD = (uint8_t)c;
  while (NRF_UART0->EVENTS_TXDRDY!=1)
  {
  }

  NRF_UART0->EVENTS_TXDRDY=0;
}

void send_array(uint8_t * array, uint16_t size)
{
	for(uint16_t i = 0; i < size; i++) 
	{
		send_byte(array[i]);
	}
}

void UART0_IRQHandler(void)
{
	if ((NRF_UART0->EVENTS_RXDRDY == 1) &&
		  (NRF_UART0->INTENSET & UART_INTENSET_RXDRDY_Msk)) {
				NRF_UART0->EVENTS_RXDRDY = 0;
				uartRxBuffer[rxBufferSize] = NRF_UART0->RXD;
				rxBufferSize++;
				if(rxBufferSize >= HEADER_SIZE) {
					nrf_gpio_pin_toggle(LED_4);
					uint16_t size = (uartRxBuffer[1] << 8) + uartRxBuffer[2];
					if(rxBufferSize == size + HEADER_SIZE) {
						MainHeader_t mainHeader = (MainHeader_t)uartRxBuffer[0];
						switch (mainHeader)
						{
							case CHANGE_MODE:
							case GET_FIRMWARE_VERSION:
							{
								char * fwVersion = "0.0.1";
								unsigned char data[] = {GET_FIRMWARE_VERSION, 0, strlen(fwVersion)};
								send_array(data, sizeof(data));
								send_array(fwVersion,strlen(fwVersion));
								break;
							}
							case GET_HARDWARE_VERSION:
							{
								char * hwVersion = "EZ1";
								unsigned char data[] = {GET_HARDWARE_VERSION, 0, strlen(hwVersion)};
								send_array(data, sizeof(data));
								send_array(hwVersion,strlen(hwVersion));
								break;
							}
							case GET_SERIAL_NUMBER:
							{
								char serialNumber[6];
								for(int i = 0; i < 4; i++)
									serialNumber[i] = NRF_FICR->DEVICEADDR[0] >> (i*8);
								for(int i = 0; i < 2 ; i++)
									serialNumber[4 + i] = NRF_FICR->DEVICEADDR[1] >> (i*8);
								
								char buffer[128];
								sprintf(buffer,"%02X:%02X:%02X:%02X:%02X:%02X", 
												serialNumber[5], serialNumber[4], serialNumber[3],
												serialNumber[2], serialNumber[1], serialNumber[0]);
								unsigned char data[] = {GET_SERIAL_NUMBER, 0, strlen(buffer)};
								send_array(data, sizeof(data));
								send_array(buffer, strlen(buffer));
							}
							case GET_BATTERY:
							case TRANSMITTER:
							case RECEIVER:
							case HUB:
							case LOCAL_UPGRADE:
							case NAK:
							case ACK:
							default:
								break;
						}
						rxBufferSize = 0;//Reset receive buffer
					}
				}
			}
}

/** @brief Function for handling the GPIOTE interrupt which is triggered on pin 0 change.
 */
void GPIOTE_IRQHandler(void)
{
    // Event causing the interrupt must be cleared.
    if ((NRF_GPIOTE->EVENTS_IN[0] == 1) && 
        (NRF_GPIOTE->INTENSET & GPIOTE_INTENSET_IN0_Msk))
    {
        NRF_GPIOTE->EVENTS_IN[0] = 0;
				nrf_gpio_pin_toggle(LED_0);
				nrf_gpio_pin_clear(LED_2);
				transmitter ^= 1;
    }
		if ((NRF_GPIOTE->EVENTS_IN[1] == 1) && 
        (NRF_GPIOTE->INTENSET & GPIOTE_INTENSET_IN1_Msk))
    {
        NRF_GPIOTE->EVENTS_IN[1] = 0;
				nrf_gpio_pin_toggle(LED_1);
				nrf_gpio_pin_clear(LED_3);
				listener ^= 1;
		}
}

void display_recv_packet(void) {
	printf("RX: %x:%x:%x:%x:%x:%x\n\r", recv_packet[3], recv_packet[4], recv_packet[5], recv_packet[6], recv_packet[7], recv_packet[8]);
}

void RADIO_IRQHandler(void) 
{
	if((NRF_RADIO->EVENTS_DISABLED) && 
		(NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk)) {
			NRF_RADIO->EVENTS_DISABLED = 0;
	}
	if((NRF_RADIO->EVENTS_END) && 
		  (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)) {
		NRF_RADIO->EVENTS_END = 0;
		if(NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle) {
			nrf_gpio_pin_toggle(LED_3);
			display_recv_packet();
		}
	}
}

void TIMER1_IRQHandler(void) 
{
	if((NRF_TIMER1->EVENTS_COMPARE[0]) &&
		  (NRF_TIMER1->INTENSET & TIMER_INTENSET_COMPARE0_Msk)) {
				NRF_TIMER1->EVENTS_COMPARE[0] = 0;
				timerEnabled = false;
	}
}

void low_power_sleep_ms(uint16_t time_in_ms) 
{
	NRF_TIMER1->CC[0] = time_in_ms*31;
	NRF_TIMER1->CC[0] += time_in_ms/4;
	NRF_TIMER1->TASKS_CLEAR = 1;
	NRF_TIMER1->TASKS_START = 1;
}

int main(void) 
{
		//bool switchMode = false;
		uint8_t packet[37];  /**< Packet to transmit. */
		uint16_t time_in_ms = DEFAULT_SLEEP_TIME_IN_MS;

	  gpio_init();
	  hfc_init();
		radio_init();
		timer1_init();
		uart_init();
		//send_byte('T');
		NRF_UART0->EVENTS_RXDRDY = 0;
		nrf_gpio_pin_set(LED_7);
		uint8_t channels[] = {2, 26, 80};
		uint8_t blechannels[] = {37, 38, 39};
		uint8_t index = 0;
		packet[0] = 0x40;//ADV_IND w/ random S0
		packet[1] = 0x24;//Size 36 bytes LENGTH
		packet[2] = 0x00;//Empty field for S1
		index += 3;
		for(int i = 0; i < 4; i++)
			packet[3 + i] = NRF_FICR->DEVICEADDR[0] >> (i*8);
		for(int i = 0; i < 2 ; i++)
			packet[7 + i] = NRF_FICR->DEVICEADDR[1] >> (i*8);
		index +=6;
		memcpy(&packet[index], advinfo, sizeof(advinfo));
		index += sizeof(advinfo);
		memcpy(&packet[index], mfginfo, sizeof(mfginfo));
		//memset(&packet[index], 0, sizeof(mfginfo));
		index += sizeof(mfginfo);
		memcpy(&packet[index], major, sizeof(major));
		index += sizeof(major);
		memcpy(&packet[index], minor, sizeof(minor));
		index += sizeof(minor);
		packet[index] = txPower;
		
    while(true) {
				if(transmitter) {
					NRF_RADIO->PACKETPTR  = (uint32_t)packet;
					NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
															(RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos);
					for(int j = 0; j < sizeof(channels); j++)
					{
						
						NRF_RADIO->FREQUENCY = channels[j];          
						NRF_RADIO->DATAWHITEIV = blechannels[j] + 0x40;
						NRF_RADIO->EVENTS_READY = 0U;
						NRF_RADIO->TASKS_TXEN   = 1; // Enable radio and wait for ready.
						__WFI();
						//Need to sleep at least 150 us for BLE
						NRF_TIMER1->CC[0] = 5;
						NRF_TIMER1->TASKS_CLEAR = 1;
						NRF_TIMER1->TASKS_START = 1;
						timerEnabled = true;
						while(timerEnabled)
							__WFI();
					}
					nrf_gpio_pin_toggle(LED_2);
				}
				//Go into listening mode on BLE channel 1
				if(listener) {
					NRF_RADIO->PACKETPTR  = (uint32_t)recv_packet;
					NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos);				
					NRF_RADIO->FREQUENCY = channels[0];
					NRF_RADIO->DATAWHITEIV = blechannels[0] + 0x40;
					NRF_RADIO->EVENTS_READY = 0U;
					NRF_RADIO->TASKS_RXEN = 1;
				}
				low_power_sleep_ms(time_in_ms);
				timerEnabled = true;
				while(timerEnabled)
					__WFI();
				NRF_RADIO->TASKS_DISABLE = 1;
		}
}

