#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "adc.h"
#include "wifi.h"
#include "schedule.h"
#include "serial.h"
#include "time.h"
#include "ble.h"
#include "JSON/jsmn.h"
#include "WifiDriver/wlan.h"
#include "WifiDriver/evnt_handler.h"
#include "WifiDriver/socket.h"
#include "WifiDriver/host_driver_version.h"
#include "WifiDriver/nvmem.h"
#include "WifiDriver/netapp.h"
#include "WifiDriver/security.h"
//char * serverAddress = "boiling-shelf-9562.herokuapp.com";
char * serverAddress = "192.168.1.17";

/* Static connection analytics variables */
float assocTime = 0.0;
float dnsTime = 0.0;
float connectTime  = 0.0;

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT
} HttpType;


//Assume that payload is a string
bool httpOperation(HttpType httpType, char * payload) {
    char txBuffer[1024];
    if(httpType == HTTP_GET)
        strcpy(txBuffer, "GET /v1/device/test HTTP/1.1\r\n");
    else if(httpType == HTTP_PUT)
        strcpy(txBuffer, "PUT /v1/device/test HTTP/1.1\r\n");
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
    startTimer();
    gethostbyname(serverAddress, strlen(serverAddress), &ip);
    dnsTime = endTimerAndGetResult();
    addr.sin_addr.s_addr = htonl(ip); 
    startTimer();
    if(connect(sock, (sockaddr *)&addr, sizeof(sockaddr_in)) == -1) {
        debug_print("Error on connect");
        closesocket(sock);
        return false;
    }
    connectTime = endTimerAndGetResult();
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
    char rxBuffer[1024];
    memset(rxBuffer, 0, 1024);
    int i = 0;
    for(i = 0; i < 1024; i++) {
        int retval = recv(sock, &rxBuffer[i], 1, 0);
        if(retval == -1) {
            debug_print("Error on recv");
            i = 1024;
        }
        else { 
            if (NULL != strstr(rxBuffer,"\r\n\r\n")) {
                debug_print("Header End Found!");
                break;
            }
        }
    }
    if(i == 1024) {
        debug_print("Error trying to recieve header");
        closesocket(sock);
        return false;

    }
    debug_print("Parse Header");
    char * contentLength = strstr(rxBuffer, "Content-Length: ");
    if(NULL != contentLength) {
        char * end = strstr(contentLength, "\r\n");
        unsigned long size = strtoul(contentLength + strlen("Content-Length: "), &end, 10);
        int bytesRx = 0;
        while(bytesRx < size) {
            if(size - bytesRx > 80) {
                recv(sock, &payload[bytesRx], 80, 0);
                bytesRx += 80;
            }
            else {
                recv(sock, &payload[bytesRx], size - bytesRx, 0);
                bytesRx = size;
            }
        }
        payload[size] = '\0';
    }
    else {
        payload[0] = '\0';
    }
    debug_print("Received HTTP Response");
    closesocket(sock);
    return true;
}


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
    ADD_JSON_STRING("macAddress", getWifiMac());
    JSON_NEXT_RECORD();
    ADD_JSON_STRING("serialNumber", "33sf3");
    JSON_NEXT_RECORD();
    ADD_JSON_STRING("softwareVersion", VERSION);
    JSON_NEXT_RECORD();
    ADD_JSON_STRING("revision", "1");
    JSON_NEXT_RECORD();
    ADD_JSON_STRING("model", "SB1");
    JSON_NEXT_RECORD();
    ADD_JSON_NUMBER("systemTime", getSystemTime());
    JSON_NEXT_RECORD();
    ADD_JSON_FLOAT("connectToAPTime", assocTime);
    JSON_NEXT_RECORD();
    ADD_JSON_FLOAT("resolveDNSTime", dnsTime);
    JSON_NEXT_RECORD();
    ADD_JSON_FLOAT("connectToServerTime", connectTime); 
    JSON_NEXT_RECORD();
    ADD_JSON_NUMBER("uptime", getUptime());
    JSON_NEXT_RECORD();
    ADD_JSON_FLOAT("batteryVoltage", getBattVoltage());
    JSON_NEXT_RECORD();
    ADD_JSON_NUMBER("beaconTransmits", getBeaconCount());
    END_JSON_OBJECT();
}

/*struct PermanentSystemInfo
{
    uint8_t model;
    uint8_t revision;
} permanentSystemInfo __attribute((section(".perm")));*/

bool reportStatusToServer(void)
{
    char txBuffer[2048];
    getSystemInfoJson(txBuffer);
    httpOperation(HTTP_PUT, txBuffer);
    return true;
}

bool parseJSONObject(char * buffer, const int size);
bool getStatusFromServer(void)
{
    char rxBuffer[2048];
    memset(rxBuffer, 0, sizeof(rxBuffer));
    if(false == httpOperation(HTTP_GET, rxBuffer))
        return false;
    return parseJSONObject(rxBuffer, strlen(rxBuffer));
}


//TODO: This is ugly and I hate it
bool parseJSONObject(char * buffer, const int size) {
    jsmn_parser jsonParse;
    jsmntok_t tokenArray[128];
    jsmn_init(&jsonParse);
    int retval = jsmn_parse(&jsonParse, buffer, size, tokenArray, 128);
    if(retval < 0) {
        sprintf(serialBuffer, "Error parsing JSON String: %d", retval);
        debug_print(serialBuffer);
        return false;
    }
    char activeKey[32];
    char activeValue[128];
    bool keyFound = false;
    int arrayObjects = 0;
    int arrayObjectSize = 0;
    bool beaconEnabled = false;
    int beaconPeriod = 0;
    int beaconPower = 0;
    int transitionTime = 0;
    resetSchedule(); //Reset the schedule and rebuild it
    for(int i = 0; i < retval; i++) {
        const int objectSize = tokenArray[i].end - tokenArray[i].start;
        if(arrayObjects > 0 && tokenArray[i].type == JSMN_OBJECT) {
            arrayObjectSize = tokenArray[i].size/2;
            keyFound = false;
        }
        else if(arrayObjects > 0 && true == keyFound) {
            arrayObjectSize--;
            if(arrayObjectSize == 0) {
                arrayObjects--;
            }
            memcpy(activeValue, &buffer[tokenArray[i].start], objectSize);
            activeValue[objectSize] = '\0';
            if(0 == strcmp(activeKey, "beaconPeriod")) {
                beaconPeriod = atoi(activeValue);
            }
            else if(0 == strcmp(activeKey, "beaconPower")) {
                beaconPower = atoi(activeValue);
            }
            else if(0 == strcmp(activeKey, "beaconEnabled")) {
                if(0 == strcmp(activeValue, "true")) {
                    beaconEnabled = true;
                }
                else {
                    beaconEnabled = false;
                }
            }
            else if(0 == strcmp(activeKey, "transitionTime")) {
                transitionTime = atoi(activeValue);
            }
            keyFound = false;
            if(arrayObjectSize == 0) {
                if(beaconEnabled) {
                    addScheduleRecord(transitionTime, true, beaconPower, beaconPeriod);
                }
                else {
                    addScheduleRecord(transitionTime, false, 0, 0);
                }
            }
        }
        else if(true == keyFound) {
            if(tokenArray[i].type == JSMN_ARRAY) {
                if(0 == strcmp(activeKey, "currentSchedule")) {
                    sprintf(serialBuffer, "Schedule size: %d", tokenArray[i].size);
                    debug_print(serialBuffer);
                    arrayObjects = tokenArray[i].size;
                }
            }
            else {
                memcpy(activeValue, &buffer[tokenArray[i].start], objectSize);
                activeValue[objectSize] = '\0';
                if(0 == strcmp(activeKey, "systemTime")) {
                    setSystemTime(atol(activeValue));
                    sprintf(serialBuffer, "systemTime: %ld", atol(activeValue));
                    debug_print(serialBuffer);
                }
                else if(0 == strcmp(activeKey, "beaconUUID")) {
                    int beaconSize = 0;
                    char * pch = strtok(activeValue, ":"); //NOTE: This changes the underlying string
                    unsigned char beaconPayload[64];
                    while(pch != NULL && beaconSize < sizeof(beaconPayload)) {
                        beaconPayload[beaconSize] = strtoul(pch, NULL, 16);  
                        pch = strtok(NULL,":");
                        beaconSize++;
                    }
                    setBLEBeaconPayload(beaconPayload, beaconSize);
                    sprintf(serialBuffer, "BeaconUUID: ");
                    for(int n = 0; n < beaconSize; n++) {
                        sprintf(serialBuffer, "%s%02x ", serialBuffer, beaconPayload[n]);
                    }
                    debug_print(serialBuffer);
                }
                else if(0 == strcmp(activeKey, "serverReconnectPeriod")) {
                    setServerReconnectPeriod(atol(activeValue));
                }
                else if(0 == strcmp(activeKey, "serverReconnectTOD")) {
                    sprintf(serialBuffer, "ServerReconnectTOD: %ld", atol(activeValue));
                    debug_print(serialBuffer);
                }
                else {
                    sprintf(serialBuffer, "Unknown key: %s", activeKey);
                    debug_print(serialBuffer);
                }
            }
            keyFound = false;
        }
        else if(tokenArray[i].type == JSMN_STRING) {
            memcpy(activeKey, &buffer[tokenArray[i].start], objectSize);
            activeKey[objectSize] = '\0';
            keyFound = true;
        }
    }
    debug_print("Finished Parsing");
    return true;
}

bool connectToWifi(void) {
    wifi_init();
    debug_print("Waiting for connection");
    startTimer();
    waitForDHCP();
    assocTime = endTimerAndGetResult();
    return true;
}



