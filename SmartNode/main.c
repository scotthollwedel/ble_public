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
        strcpy(txBuffer, "GET /v1/devices HTTP/1.1\r\n");
    else if(httpType == HTTP_PUT)
        strcpy(txBuffer, "PUT /v1/devices HTTP/1.1\r\n");
    strcat(txBuffer, "Host: ");
    strcat(txBuffer, serverAddress);
    strcat(txBuffer, "\r\n");
    if(payload != NULL) {
        sprintf(&txBuffer[strlen(txBuffer)], "Content-Type: application/json\r\nContent-Length: %d\r\n", strlen(payload));
    }
    strcat(txBuffer, "Accept: application/json,text/html\r\n");
    strcat(txBuffer, "X-Requested-With: XMLHttpRequest\r\n");
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

/* Static system analytics variables */
float assocTime = 0.0;
long serverConnectionCount = 0;

#define START_JSON_OBJECT_UNNAMED() size += sprintf(&txBuffer[size], "{")
#define ADD_JSON_STRING(x,y) size += sprintf(&txBuffer[size], "\"%s\": \"%s\"",x,y)
#define START_JSON_OBJECT(x) size += sprintf(&txBuffer[size], "\"%s\": {", x)
#define END_JSON_OBJECT() size += sprintf(&txBuffer[size], "}")
#define START_JSON_ARRAY(x) size += sprintf(&txBuffer[size], "\"%s\": [", x)
#define END_JSON_ARRAY() size += sprintf(&txBuffer[size], "]")
#define ADD_JSON_NUMBER(x,y) size += sprintf(&txBuffer[size], "\"%s\": \"%ld\"",x,y)
#define ADD_JSON_FLOAT(x,y) size += sprintf(&txBuffer[size], "\"%s\": \"%.3f\"",x,y)
#define JSON_NEXT_RECORD() size += sprintf(&txBuffer[size], ",");
void getSystemInfoJson(char * txBuffer)
{
    int size = 0;
    START_JSON_OBJECT_UNNAMED();
    ADD_JSON_STRING("macAddress", wifiMacAddress);
    JSON_NEXT_RECORD();
    ADD_JSON_STRING("serialNumber", "33sf3");
    JSON_NEXT_RECORD();
    ADD_JSON_STRING("softwareVersion", VERSION);
    JSON_NEXT_RECORD();
    ADD_JSON_STRING("revision", "1");
    JSON_NEXT_RECORD();
    ADD_JSON_STRING("model", "SB1");
    JSON_NEXT_RECORD();
    ADD_JSON_NUMBER("systemTime", 0L);
    JSON_NEXT_RECORD();
    ADD_JSON_FLOAT("connectToAPTime", assocTime);
    JSON_NEXT_RECORD();
    ADD_JSON_FLOAT("resolveDNSTime", 1.2);
    JSON_NEXT_RECORD();
    ADD_JSON_FLOAT("connectToServerTime", 2.3); 
    JSON_NEXT_RECORD();
    ADD_JSON_NUMBER("uptime", getUptime());
    JSON_NEXT_RECORD();
    ADD_JSON_FLOAT("batteryVoltage", getBattVoltage());
    JSON_NEXT_RECORD();
    ADD_JSON_NUMBER("beaconTransmits", getBeaconCount());
    JSON_NEXT_RECORD();
    ADD_JSON_NUMBER("serverConnections",serverConnectionCount);
    END_JSON_OBJECT();
}

/*struct PermanentSystemInfo
{
    uint8_t model;
    uint8_t revision;
} permanentSystemInfo __attribute((section(".perm")));*/

bool reportStatusToServer(void)
{
    wifi_init();
    debug_print("Waiting for connection");
    struct tm startTime;
    struct tm endTime;
    getTime(&startTime);
    waitForDHCP();
    getTime(&endTime);
    float startTimeNum = (float)startTime.sec + ((float)startTime.usec/1000000);
    float endTimeNum = (float)endTime.sec + ((float)endTime.usec/1000000);
    assocTime = endTimeNum - startTimeNum;

    char txBuffer[2048];
    getSystemInfoJson(txBuffer);
    httpOperation(HTTP_PUT, txBuffer);
    wlan_stop();
    return true;
}

bool reportStatus = true;
void setReportStatus()
{
    reportStatus = true;
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
    debug_print("Connect to server");
    while(true) {
        if(reportStatus) {
            reportStatus = false;
            serverConnectionCount++;
            if(true == reportStatusToServer()) {
                resetBeaconCount();
            }
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

