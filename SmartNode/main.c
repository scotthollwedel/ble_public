#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "boards.h"
#include "gpio.h"
#include "spi.h"
#include "nrf_delay.h"
#include "serial.h"
#include "time.h"
#include "ble.h"
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
    debug_print("Starting smart config");
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
    debug_print("Restarting Wifi");
    nrf_delay_ms(1000);
	wlan_start(0);
	// Mask out all non-required events
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE|HCI_EVNT_WLAN_UNSOL_INIT|HCI_EVNT_WLAN_ASYNC_PING_REPORT);
    while (ulCC3000DHCP == 0); // Wait for DHCP

    for(unsigned int index = 0; index < 3; index++)
        mdnsAdvertiser(1,device_name,strlen(device_name));
}

char wifiMacAddress[] = "00:00:00:00:00:00";
unsigned char wifiVersion[2];
//char * serverAddress = "boiling-shelf-9562.herokuapp.com";
char * serverAddress = "192.168.1.17";

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT
} HttpType;


//Assume that payload is a string
void httpOperation(HttpType httpType, char * payload) {
    char txBuffer[1024];
    char rxBuffer[1024];
    memset(rxBuffer, 0, sizeof(rxBuffer));
    if(httpType == HTTP_GET)
        strcpy(txBuffer, "GET / HTTP/1.1\r\n");
    else if(httpType == HTTP_PUT)
        strcpy(txBuffer, "PUT / HTTP/1.1\r\n");
    strcat(txBuffer, "Host: ");
    strcat(txBuffer, serverAddress);
    strcat(txBuffer, "\r\n");
    if(payload != NULL) {
        sprintf(&txBuffer[strlen(txBuffer)], "Content-Type: application/json\r\nContent-Length: %d\r\n", strlen(payload));
    }
    strcat(txBuffer, "Accept: application/json,text/html\r\n");
    strcat(txBuffer, "\r\n");
    long sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    unsigned long recvTimeout = 10000;//Time in ms
    setsockopt(sock,SOL_SOCKET, SOCKOPT_RECV_TIMEOUT, &recvTimeout, sizeof(recvTimeout));
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    //addr.sin_port = htons(80);
    addr.sin_port = htons(5000);
    unsigned long ip = 0;
    debug_print("Looking up DNS address");
    gethostbyname(serverAddress, strlen(serverAddress), &ip);
    addr.sin_addr.s_addr = htonl(ip); 
    connect(sock, (sockaddr *)&addr, sizeof(sockaddr_in));
    debug_print("Sending HTTP Request");
    const int txLength = strlen(txBuffer);
    int bytesSent = 0;
    while(bytesSent < txLength) {
        if(txLength - bytesSent > CC3000_MINIMAL_TX_SIZE) {
            send(sock, &txBuffer[bytesSent], CC3000_MINIMAL_TX_SIZE, 0);
            bytesSent += CC3000_MINIMAL_TX_SIZE;
        }
        else {
            send(sock, &txBuffer[bytesSent], txLength - bytesSent, 0);
            bytesSent = txLength;
        }
    }
    if(payload != NULL) {
        const int txLength = strlen(payload);
        int bytesSent = 0;
        while(bytesSent < txLength) {
            if(txLength - bytesSent > CC3000_MINIMAL_TX_SIZE) {
                send(sock, &payload[bytesSent], CC3000_MINIMAL_TX_SIZE, 0);
                bytesSent += CC3000_MINIMAL_TX_SIZE;
            }
            else {
                send(sock, &payload[bytesSent], txLength - bytesSent, 0);
                bytesSent = txLength;
            }
        }
    }
    debug_print("Waiting for Response");
    //Get HTTP Header
    int i = 0;
    for(i = 0; i < sizeof(rxBuffer); i++) {
        recv(sock, &rxBuffer[i], 1, 0);
        if (NULL != strstr(rxBuffer,"\r\n\r\n")) {
            break;
        }
    }
    char * header = strstr(rxBuffer, "Content-Length: ");
    char * end = strstr(header, "\r\n");
    unsigned long size = strtoul(header + strlen("Content-Length: "), &end, 10);
    recv(sock, &rxBuffer[i], size, 0);
    debug_print("Received HTTP Response");
    closesocket(sock);
    debug_print("Payload Received");
    debug_print(&rxBuffer[i]);
}

float assocTime = 0.0;

#define ADD_JSON_STRING(x,y) size += sprintf(&txBuffer[size], "\"%s\": \"%s\"",x,y)
#define START_JSON_OBJECT(x) size += sprintf(&txBuffer[size], "\"%s\": {", x)
#define END_JSON_OBJECT() size += sprintf(&txBuffer[size], "}")
#define ADD_JSON_NUMBER(x,y) size += sprintf(&txBuffer[size], "\"%s\": \"%ld\"",x,y)
#define ADD_JSON_FLOAT(x,y) size += sprintf(&txBuffer[size], "\"%s\": \"%.3f\"",x,y)
void getSystemInfoJson(char * txBuffer)
{
    int size = 0;
    size += sprintf(&txBuffer[size], "{");
    ADD_JSON_STRING("macAddress", wifiMacAddress);
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_STRING("serialNumber", "33sf3");
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_STRING("softwareVersion", VERSION);
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_STRING("revision", "1");
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_STRING("model", "SB1");
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_NUMBER("systemTime", 0L);
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_FLOAT("connectToAPTime", assocTime);
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_NUMBER("connectToServerTime", 1000L); 
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_NUMBER("uptime", 234L);
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_STRING("voltage", "3.0"); //Not sure how this is going to work yet..need float support
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_NUMBER("serverReconnectPeriod", 10324L);
    size += sprintf(&txBuffer[size], ",");
    START_JSON_OBJECT("schedule");
    ADD_JSON_NUMBER("startTime", 0L);
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_NUMBER("beaconPeriod", 1000L);
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_NUMBER("beaconPower", 0L);
    size += sprintf(&txBuffer[size], ",");
    ADD_JSON_STRING("beaconUUID", "ab:cd:ed:12:34:12:f2:9e:88:77");
    END_JSON_OBJECT();
    sprintf(&txBuffer[size], "}");
}

/*struct PermanentSystemInfo
{
    uint8_t model;
    uint8_t revision;
} permanentSystemInfo __attribute((section(".perm")));*/

int main(void) 
{
    clock_init();
    gpio_init();
    uart_init();
    rtc_init();
    timer0_init();
    ble_init();
    debug_print("Application Start");
    /*unsigned long onPeriod = 1;
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
    return 0; */
    spi_init();
    wifi_init();
    debug_print("Wifi Initialized");
    unsigned char mac[6];
    nvmem_get_mac_address(mac);
    sprintf(wifiMacAddress, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    nvmem_read_sp_version(wifiVersion);
    //StartSmartConfig();
    debug_print("Waiting for connection");
    struct tm startTime;
    getTime(&startTime);
    while (ulCC3000DHCP == 0); // Wait for DHCP
    struct tm endTime;
    getTime(&endTime);
    float startTimeNum = (float)startTime.sec + ((float)startTime.usec/1000000);
    float endTimeNum = (float)endTime.sec + ((float)endTime.usec/1000000);
    assocTime = endTimeNum - startTimeNum;
    char txBuffer[2048];
    getSystemInfoJson(txBuffer);
    debug_print(txBuffer);
    httpOperation(HTTP_PUT, txBuffer);
    while(true) {
        __WFI();
    }
}

