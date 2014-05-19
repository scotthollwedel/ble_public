#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "serial_protocol.h"
#include "ota_protocol.h"
#include "version.h"
#include "ble.h"
#include "serial.h"
#include "gpio.h"
#include "time.h"
#include "radio.h"


void clock_init()
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;
    // Wait for the external oscillator to start up. Required to use radio.
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);

    NRF_CLOCK->EVENTS_DONE = 0;
    NRF_CLOCK->TASKS_CAL = 1;
    while (NRF_CLOCK->EVENTS_DONE == 0);
}

int main(void) 
{
    clock_init();
    gpio_init();
    radio_init();
    timer0_init();
    rtc_init();
    uart_init();
    nrf_gpio_pin_set(LED_7);
    while(true) {
        __WFI();
    }
}

