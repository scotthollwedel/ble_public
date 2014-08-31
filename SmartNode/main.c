#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "boards.h"
#include "gpio.h"
#include "spi.h"
#include "nrf_delay.h"
#include "serial.h"
#include "time.h"
#include "ble.h"
#include "adc.h"
#include "wifi.h"
#include "main.h"
#include "schedule.h"
#include "backend_connection.h"
#include "WifiDriver/wlan.h"
#include "WifiDriver/evnt_handler.h"
#include "WifiDriver/socket.h"
#include "WifiDriver/host_driver_version.h"
#include "WifiDriver/nvmem.h"
#include "WifiDriver/netapp.h"
#include "WifiDriver/security.h"

/* Static system variables */
char wifiMacAddress[] = "00:00:00:00:00:00";
unsigned char wifiVersion[2];


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

bool reportStatus = false;
void setReportStatus()
{
    reportStatus = true;
}

char * getWifiMac() {
    return wifiMacAddress; 
}

int main(void) 
{
    clock_init();
    gpio_init();
    uart_init();
    rtc_init();
    timer0_init();
    ble_init();
    adc_init();
    debug_print("Application Start");
    spi_init();

    wifi_init();
    unsigned char mac[6];
    nvmem_get_mac_address(mac);
    sprintf(wifiMacAddress, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    nvmem_read_sp_version(wifiVersion);
    wlan_stop();

    nrf_delay_ms(1000);

    debug_print("Connect to server");
    connectToWifi();
    getStatusFromServer();
    wlan_stop();

    while(true) {
        if(reportStatus) {
            reportStatus = false;
            connectToWifi();
            if(true == reportStatusToServer()) {
                resetBeaconCount();
            }
            getStatusFromServer();
            wlan_stop();
        }
        __WFI();
    }
}

void ledFade(void)
{
    unsigned long onPeriod = 1;
    bool increase = true;
    #define PERIOD 1000
    while(1) {
        if(onPeriod == PERIOD - 1) {
            increase = false;
        }
        else if(onPeriod == 1) {
            increase = true;
        }
        nrf_gpio_pin_set(LED_0);
        nrf_delay_us(onPeriod);
        nrf_gpio_pin_clear(LED_0);
        nrf_delay_us(PERIOD - onPeriod);
        if(increase)
            onPeriod++;
        else
            onPeriod--;
    }
}

