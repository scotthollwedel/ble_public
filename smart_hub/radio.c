#include <stdint.h>
#include "boards.h"
#include "nrf_gpio.h"
#include "serial.h"
#include "hub.h"
#include "serial_protocol.h"
#include "radio.h"

extern uint8_t * hub_rx;

void radio_init()
{
    NRF_RADIO->INTENSET = (RADIO_INTENSET_DISABLED_Enabled << RADIO_INTENSET_DISABLED_Pos) | (RADIO_INTENSET_END_Enabled << RADIO_INTENSET_END_Pos); 
    NVIC_EnableIRQ(RADIO_IRQn);
}

unsigned int channel_index = 0;

void RADIO_IRQHandler(void) 
{
    if((NRF_RADIO->EVENTS_DISABLED) && (NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk)) 
    {
        NRF_RADIO->EVENTS_DISABLED = 0;
        //Switch over to receive mode
        NRF_RADIO->PACKETPTR = (uint32_t)hub_rx;
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_RXEN = 1;
    }

    if((NRF_RADIO->EVENTS_END) && (NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk)) 
    {
        NRF_RADIO->EVENTS_END = 0;
        if(NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle) {
            handlePacket();
        }
    }
}

void sendPacket(const uint8_t power, const uint8_t frequency)
{
    NRF_RADIO->TXPOWER = power;
    NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) | (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos);
    NRF_RADIO->FREQUENCY = frequency;          
    NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN   = 1;
}