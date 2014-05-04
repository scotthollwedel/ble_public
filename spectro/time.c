#include <stdint.h>
#include "boards.h"
#include "nrf_gpio.h"
#include "time.h"
#include "ble.h"
#include "hub.h"
#include "state_variables.h"
#include "radio.h"

#define TICKS_TO_RX LF_CLOCK_PERIOD/128

static unsigned long count = 0;
static unsigned int packet[255];

extern uint8_t packetQueueHead;
extern uint8_t packetQueueTail;
extern uint8_t * packetQueue;
extern uint8_t * pilot;

//Configuration RTC with 
void rtc_init()
{
	NRF_RTC0->CC[0] = LF_CLOCK_PERIOD*LOOP_PERIOD_IN_MS/1000;
	NRF_RTC0->INTENSET = (RTC_INTENSET_COMPARE0_Enabled << RTC_INTENSET_COMPARE0_Pos);
	NRF_RTC0->TASKS_START = 1;
	NVIC_EnableIRQ(RTC0_IRQn);
}

//Configuration TIMER0 with a 31250 us period
void timer0_init()
{
    NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER0->PRESCALER = 0; //16000000/2^0 = 16000000 Ticks/Sec
    NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_16Bit; //Roll over at 2^16 = 65536. Max bitmode is 16 bit.
    NRF_TIMER0->SHORTS = (TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos) | 
                         (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
    NRF_TIMER0->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
    NVIC_EnableIRQ(TIMER0_IRQn);
}

void ModeTransition()
{
    SystemMode_t newMode = getNewMode();
    SystemMode_t mode = getMode();
    if(newMode != mode) {
        setMode(newMode);
        switch(newMode) {
            case IDLE_MODE:
                break;
            case TRANSMITTER_MODE:
                ble_init();
                NRF_RADIO->PACKETPTR  = (uint32_t)packet;                
                break;
            case RECEIVER_MODE:
                ble_init();
                NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos);				
                NRF_RADIO->FREQUENCY = getBLERFChannel(1);
                NRF_RADIO->DATAWHITEIV = getBLELogicalChannel(1) + 0x40;
                NRF_RADIO->EVENTS_READY = 0U;
                NRF_RADIO->TASKS_RXEN = 1;
                NRF_RADIO->PACKETPTR  = (uint32_t)packet;
                break;
            case HUB_MODE:
                hub_init();
                break;
        }
    }
}

void HandleModePeriodicBehavior()
{
    SystemMode_t mode = getMode();
    switch(mode) {
        case IDLE_MODE:
            break;
        case TRANSMITTER_MODE:
            if(((count != 0) && (count % getBLEBeaconTransmitPeriod())) == 0) {
                sendBLEBeacon(0);
            }
            break;
        case RECEIVER_MODE:
            break;
        case HUB_MODE:
            if(((count != 0) && (count % HUB_BEACON_PERIOD)) == 0) {
                if(packetQueueHead != packetQueueTail) {
                    NRF_RADIO->PACKETPTR = (uint32_t)packetQueue[packetQueueHead];
                    packetQueueHead++;
                    packetQueueHead |= 16 - 1;
                    sendPacket(RADIO_TXPOWER_TXPOWER_Pos4dBm, 24);
                }
                else {
                    setPilot();
                    NRF_RADIO->PACKETPTR = (uint32_t)pilot;
                    sendPacket(RADIO_TXPOWER_TXPOWER_Pos4dBm, 24);
                }
                NRF_TIMER0->CC[0] = TICKS_TO_RX;
                NRF_TIMER0->TASKS_START = 1;
            }
            break;
    }
}

void RTC0_IRQHandler(void)
{
	if((NRF_RTC0->EVENTS_COMPARE[0]) && 
		 (NRF_RTC0->INTENSET & RTC_INTENSET_COMPARE0_Msk)) {
		NRF_RTC0->EVENTS_COMPARE[0] = 0;
             
        //Change operating mode
        ModeTransition();
        HandleModePeriodicBehavior();
             
        NRF_RTC0->CC[0] += LF_CLOCK_PERIOD*LOOP_PERIOD_IN_MS/1000;
        count++;
        //nrf_gpio_pin_toggle(LED_2);
	}
}

void TIMER0_IRQHandler(void) 
{
	if((NRF_TIMER0->EVENTS_COMPARE[0]) &&
		  (NRF_TIMER0->INTENSET & TIMER_INTENSET_COMPARE0_Msk)) 
    {
        NRF_TIMER0->EVENTS_COMPARE[0] = 0;
         SystemMode_t mode = getMode();
        if(mode == TRANSMITTER_MODE) {
            //Start transmit for BLE transmit mode after correct time
            NRF_RADIO->EVENTS_READY = 0U;
            NRF_RADIO->TASKS_TXEN   = 1;
        }
        else if(mode == HUB_MODE) {
            NRF_RADIO->TASKS_DISABLE = 1;//Turn off the radio
        }
	}
}
