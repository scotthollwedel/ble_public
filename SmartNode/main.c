#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "boards.h"
#include "gpio.h"
#include "spi.h"
#include "nrf_delay.h"
#include "serial.h"
#include "WifiDriver/wlan.h"
#include "WifiDriver/evnt_handler.h"
#include "WifiDriver/socket.h"
#include "WifiDriver/host_driver_version.h"
#include "WifiDriver/nvmem.h"
#include "WifiDriver/netapp.h"
#include "WifiDriver/security.h"

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

#define NETAPP_IPCONFIG_MAC_OFFSET              (20)
// Simple Config Prefix
const char aucCC3000_prefix[] = {'T', 'T', 'T'};
volatile unsigned long ulSmartConfigFinished, ulCC3000Connected,ulCC3000DHCP, OkToDoShutDown, ulCC3000DHCP_configured;
volatile unsigned char ucStopSmartConfig;
volatile long ulSocket;
//device name used by smart config response
char device_name[] = "CC3000";
//AES key "smartconfigAES16"
const unsigned char smartconfigkey[] = {0x73,0x6d,0x61,0x72,0x74,0x63,0x6f,0x6e,0x66,0x69,0x67,0x41,0x45,0x53,0x31,0x36};
unsigned char printOnce = 1;

char *sendWLFWPatch(unsigned long *Length)
{
    *Length = 0;
    return NULL;
}


char *sendDriverPatch(unsigned long *Length)
{
    *Length = 0;
    return NULL;
}

char *sendBootLoaderPatch(unsigned long *Length)
{
    *Length = 0;
    return NULL;
}

void CC3000_UsynchCallback(long lEventType, char * data, unsigned char length)
{
  
	if (lEventType == HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE)
	{
		ulSmartConfigFinished = 1;
		ucStopSmartConfig     = 1;  
	}
    if (lEventType == HCI_EVNT_WLAN_KEEPALIVE)
    {
        debug_print("Keep alive");
    }
	if (lEventType == HCI_EVNT_WLAN_UNSOL_CONNECT)
	{
        debug_print("Connected");
		ulCC3000Connected = 1;
	}
	
	if (lEventType == HCI_EVNT_WLAN_UNSOL_DISCONNECT)
	{		
        debug_print("Disconnected");
		ulCC3000Connected = 0;
		ulCC3000DHCP      = 0;
		ulCC3000DHCP_configured = 0;
		printOnce = 1;
	}
	
	if (lEventType == HCI_EVNT_WLAN_UNSOL_DHCP)
	{
		// Notes: 
		// 1) IP config parameters are received swapped
		// 2) IP config parameters are valid only if status is OK, i.e. ulCC3000DHCP becomes 1
		
		// only if status is OK, the flag is set to 1 and the addresses are valid
        debug_print("DHCP");
		if ( *(data + NETAPP_IPCONFIG_MAC_OFFSET) == 0)
		{
			ulCC3000DHCP = 1;
            tNetappDhcpParams * dhcpResult = (tNetappDhcpParams *)&data[0];
            char buffer[128];
            sprintf(buffer, "Assigned IP: %d.%d.%d.%d",dhcpResult->aucIP[3], dhcpResult->aucIP[2],dhcpResult->aucIP[1],dhcpResult->aucIP[0]);
            debug_print(buffer);
		}
		else
		{
			ulCC3000DHCP = 0;
		}
		
	}
	if (lEventType == HCI_EVENT_CC3000_CAN_SHUT_DOWN)
	{
        debug_print("Disconnected");
		OkToDoShutDown = 1;
	}
}

long ReadWlanInterruptPin(void)
{
    // Return the status of IRQ, P2.4 
    return gpio_get_cc3000_irq();
}

void WlanInterruptEnable()
{
    gpio_enable_cc3000_irq();
}

void WlanInterruptDisable()
{
    gpio_disable_cc3000_irq();
}

void WriteWlanPin(uint8_t trueFalse)
{   
    if(trueFalse) {
        //Power up CC3000
        gpio_power_up_cc3000();
    }
    else {
        gpio_power_down_cc3000();
    }
}

void wifi_init(void)
{
    ulCC3000DHCP = 0;
    ulCC3000Connected = 0;
    ulSocket = 0;
    ulSmartConfigFinished = 0;

    //WLAN on API Implementation
    wlan_init( CC3000_UsynchCallback, sendWLFWPatch, sendDriverPatch, sendBootLoaderPatch, ReadWlanInterruptPin, WlanInterruptEnable, WlanInterruptDisable, WriteWlanPin);
    wlan_start(0);
    wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE|HCI_EVNT_WLAN_UNSOL_INIT|HCI_EVNT_WLAN_ASYNC_PING_REPORT);
}

void scan_wifi(void) {
    unsigned long intervalList[] = {2000, 2000, 2000, 2000,
                                    2000, 2000, 2000, 2000,
                                    2000, 2000, 2000, 2000,
                                    2000, 2000, 2000, 2000};
    if(wlan_ioctl_set_scan_params(1, 100, 100, 5, 0x7ff, -80, 0, 205, intervalList) != 0) {
        debug_print("ERROR!");
    }
    debug_print("scan complete");
    /*unsigned char results_buffer[128];
    char buffer[512];
    
    uint32_t networksFound = 0xff;
    while(networksFound > 1) {
        wlan_ioctl_get_scan_results(0, results_buffer);
        char * results_ptr = (char *)results_buffer;
        STREAM_TO_UINT32(results_ptr,0,networksFound)
        sprintf(buffer, "Networks Found: %lu",networksFound);
        debug_print(buffer);
        results_ptr += 4;
        results_ptr += 4;
        sprintf(buffer, "Valid: %d\n\rRSSI: %d", (*results_ptr & 0x01), (*results_ptr & (0x7f << 1)) >> 1);
        debug_print(buffer);
        results_ptr++;

        int ssidLength = (*results_ptr & (0x3F << 2)) >> 2;
        sprintf(buffer, "Security Mode: %d\n\rSSID Length: %d", (*results_ptr & 0x03), ssidLength);
        debug_print(buffer);
        results_ptr++;
        results_ptr += 2;

        sprintf(buffer, "SSID: "); 
        for(int i = 0; i < 32; i++) {
            if(i < ssidLength)
                sprintf(buffer, "%s%c", buffer, *results_ptr);
            results_ptr++;
        }
        debug_print(buffer);

        sprintf(buffer, "BSSID"); 
        for(int i = 0; i < 6; i++) {
            sprintf(buffer, "%s:%02x", buffer, *results_ptr);
            results_ptr++;
        }
        debug_print(buffer);
    }*/
    //debug_print("report finished");

}

void test_connection(void)
{
    scan_wifi();
    wlan_ioctl_set_connection_policy(0,0,0);
    debug_print("Connecting...");
    /*char * ssid = "Disalcot";
    char * key = "9163374991";
    long key_len = strlen(key);*/
    char * ssid = "NETGEAR";
    long ssid_len = strlen(ssid);
    unsigned char bssid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    //if(wlan_connect(WLAN_SEC_WPA2, ssid, ssid_len, bssid, (unsigned char *)key, key_len) != 0) {
    if(wlan_connect(WLAN_SEC_UNSEC, ssid, ssid_len, bssid, NULL, 0) != 0) {
        debug_print("ERROR!");
    }
    while(ulCC3000DHCP == 0);
    long sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in toaddr;
    toaddr.sin_family = AF_INET;
    toaddr.sin_port = htons(3500);
    toaddr.sin_addr.s_addr = htonl(0xc0a80102);
    char * buffer = "hi";
    while(true) {
        sendto(sock, buffer, strlen(buffer), 0, (sockaddr *)&toaddr, sizeof(sockaddr_in));  
        nrf_delay_ms(1000);
    }

}

void StartSmartConfig(void)
{
	ulSmartConfigFinished = 0;
	ulCC3000Connected = 0;
	ulCC3000DHCP = 0;
	OkToDoShutDown=0;
	// Reset all the previous configuration
	wlan_ioctl_set_connection_policy(0, 0, 0);	
	wlan_ioctl_del_profile(255);
	//Wait until CC3000 is disconnected
	while (ulCC3000Connected == 1);
	{
        nrf_delay_ms(1);
		hci_unsolicited_event_handler();
	}
	wlan_smart_config_set_prefix((char*)aucCC3000_prefix);
	// Start the SmartConfig start process
	wlan_smart_config_start(1);
	// Wait for Smartconfig process complete
	while (ulSmartConfigFinished == 0);
	// create new entry for AES encryption key
	nvmem_create_entry(NVMEM_AES128_KEY_FILEID,16);
	// write AES key to NVMEM
	aes_write_key((unsigned char *)smartconfigkey);
	// Decrypt configuration information and add profile
    wlan_smart_config_process();
	// reset the CC3000
	wlan_ioctl_set_connection_policy(0, 0, 1);	
	wlan_stop();
    nrf_delay_ms(1000);
	wlan_start(0);
	// Mask out all non-required events
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE|HCI_EVNT_WLAN_UNSOL_INIT|HCI_EVNT_WLAN_ASYNC_PING_REPORT);
    while (ulCC3000DHCP == 0); // Wait for DHCP

    for(unsigned int index = 0; index < 3; index++)
        mdnsAdvertiser(1,device_name,strlen(device_name));
}

char wifiMacAddress[6];
char wifiVersion[2];

int main(void) 
{
    clock_init();
    nrf_delay_ms(10); //wait for power to stabilize
    uart_init();
    debug_print("Application Start");
    gpio_init();
    nrf_delay_ms(10); //wait for power to stabilize
    init_spi();
    wifi_init();
    debug_print("Wifi Initialized");
    nvmem_get_mac_address(wifiMacAddress);
    nvmem_read_sp_version(wifiVersion);
    unsigned char version[2];
    nrf_gpio_pin_set(LED_0);
    StartSmartConfig();
    nrf_gpio_pin_set(LED_1);
    while(true) {
        __WFI();
    }
}

