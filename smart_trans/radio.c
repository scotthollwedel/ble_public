#include <stdint.h>
#include "boards.h"
#include "nrf_gpio.h"
#include "serial.h"
#include "ble.h"
#include "serial_protocol.h"
#include "radio.h"
#include "time.h"

#define TICKS_BETWEEN_BLE_TRANSMIT .000150/HF_CLOCK_PERIOD

void radio_init()
{
    NRF_RADIO->INTENSET = (RADIO_INTENSET_DISABLED_Enabled << RADIO_INTENSET_DISABLED_Pos); 
    NVIC_EnableIRQ(RADIO_IRQn);
}

unsigned int channel_index = 0;

void RADIO_IRQHandler(void) 
{
	if((NRF_RADIO->EVENTS_DISABLED) && (NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk)) 
    {
        NRF_RADIO->EVENTS_DISABLED = 0;
        if(channel_index == 2) {
            channel_index = 0;
            nrf_gpio_pin_toggle(LED_1);
        }
        else {
            channel_index++;				
            NRF_RADIO->FREQUENCY = getBLERFChannel(channel_index);          
            NRF_RADIO->DATAWHITEIV = getBLELogicalChannel(channel_index) + 0x40;
            NRF_TIMER0->CC[0] = TICKS_BETWEEN_BLE_TRANSMIT;
            NRF_TIMER0->TASKS_START = 1;
        }
	}
}

void sendPacket(const uint8_t power, const uint8_t frequency)
{
    NRF_RADIO->TXPOWER = power;
    NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
                        (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos);
    NRF_RADIO->FREQUENCY = frequency;          
    NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN   = 1;
}
