#include <stdint.h>
#include "boards.h"
#include "nrf_gpio.h"
#include "serial.h"
#include "ble.h"
#include "serial_protocol.h"
#include "radio.h"

void radio_init()
{
    NRF_RADIO->INTENSET = (RADIO_INTENSET_END_Enabled << RADIO_INTENSET_END_Pos); 
    NVIC_EnableIRQ(RADIO_IRQn);
}

unsigned int channel_index = 0;

void RADIO_IRQHandler(void) 
{
	if((NRF_RADIO->EVENTS_END) && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)) 
    {
		NRF_RADIO->EVENTS_END = 0;
		if(NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle) {
            uint8_t data[] = {RECEIVED_BEACON, 1, 2};
            nrf_gpio_pin_toggle(LED_4);
            send_array(data, 4);
            //send_array(&recv_packet[2], 38);
            NRF_RADIO->TASKS_START = 1;
		}
	}
}
