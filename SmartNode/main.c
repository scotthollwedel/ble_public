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
char device_name[] = "SPECTRO"; //device name used by smart config response
const unsigned char smartconfigkey[] = {0x73,0x6d,0x61,0x72,0x74,0x63,0x6f,0x6e,0x66,0x69,0x67,0x41,0x45,0x53,0x31,0x36}; //AES key "smartconfigAES16"
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
	wlan_ioctl_set_connection_policy(0, 0, 1);	
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

unsigned char wifiMacAddress[6];
unsigned char wifiVersion[2];
char * serverAddress = "boiling-shelf-9562.herokuapp.com";

void httpGet(char * payload, int payloadSize) {
    char txBuffer[1024];
    char rxBuffer[1024];
    memset(rxBuffer, 0, sizeof(rxBuffer));
    strcpy(txBuffer, "GET / HTTP/1.1\r\n");
    strcat(txBuffer, "Host: ");
    strcat(txBuffer, serverAddress);
    strcat(txBuffer, "\r\n");
    strcat(txBuffer, "Accept: text/html\r\n");
    strcat(txBuffer, "\r\n");
    long sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    unsigned long ip = 0;
    gethostbyname(serverAddress, strlen(serverAddress), &ip);
    addr.sin_addr.s_addr = htonl(ip); 
    connect(sock, (sockaddr *)&addr, sizeof(sockaddr_in));
    debug_print("Sending GET...");
    send(sock, txBuffer, strlen(txBuffer), 0);
    //Get HTTP Header
    debug_print("Getting HTTP Header");
    int i = 0;
    for(i = 0; i < sizeof(rxBuffer); i++) {
        recv(sock, &rxBuffer[i], 1, 0);
        if (NULL != strstr(rxBuffer,"\r\n\r\n")) {
            break;
        }
    }
    debug_print("Header Received");
    char * header = strstr(rxBuffer, "Content-Length: ");
    char * end = strstr(header, "\r\n");
    unsigned long size = strtoul(header + strlen("Content-Length: "), &end, 10);
    recv(sock, &rxBuffer[i], size, 0);
    closesocket(sock);
    debug_print("Payload Received");
    debug_print(&rxBuffer[i]);
}

void httpPut(char * payload, int payloadSize) {
    char txBuffer[1024];
    char rxBuffer[1024];
    memset(rxBuffer,0, sizeof(rxBuffer));
    strcpy(txBuffer, "PUT / HTTP/1.1\r\n");
    strcat(txBuffer, "Host: ");
    strcat(txBuffer, serverAddress);
    strcat(txBuffer, "\r\n");
    strcat(txBuffer, "Accept: text/html\r\n");
    strcat(txBuffer, "\r\n");
    long sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    unsigned long ip = 0;
    gethostbyname(serverAddress, strlen(serverAddress), &ip);
    addr.sin_addr.s_addr = htonl(ip); 
    connect(sock, (sockaddr *)&addr, sizeof(sockaddr_in));
    debug_print("Sending PUT...");
    send(sock, txBuffer, strlen(txBuffer), 0);
    //Get HTTP Header
    debug_print("Getting HTTP Header");
    int i = 0;
    for(i = 0; i < sizeof(rxBuffer); i++) {
        recv(sock, &rxBuffer[i], 1, 0);
        if (NULL != strstr(rxBuffer,"\r\n\r\n")) {
            break;
        }
    }
    debug_print("Header Received");
    debug_print(rxBuffer);
    char * header = strstr(rxBuffer, "Content-Length: ");
    char * end = strstr(header, "\r\n");
    unsigned long size = strtoul(header + strlen("Content-Length: "), &end, 10);
    recv(sock, &rxBuffer[i], size, 0);
    closesocket(sock);
    debug_print("Payload Received");
    debug_print(&rxBuffer[i]);
}

int main(void) 
{
    clock_init();
    gpio_init();
    nrf_gpio_pin_set(LED_0);
    uart_init();
    debug_print("Application Start");
    init_spi();
    wifi_init();
    debug_print("Wifi Initialized");
    nvmem_get_mac_address(wifiMacAddress);
    nvmem_read_sp_version(wifiVersion);
    //StartSmartConfig();
    while (ulCC3000DHCP == 0); // Wait for DHCP
    nrf_gpio_pin_set(LED_1);
    httpPut(NULL, 0);
    while(true) {
        __WFI();
    }
}

