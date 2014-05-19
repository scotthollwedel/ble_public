#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf_gpio.h"
#include "serial_protocol.h"
#include "boards.h"
#include "serial.h"

/* UART RX BUFFER HANDLER */
static uint8_t uartRxBuffer[100];
static uint8_t rxBufferSize = 0;

void UART0_IRQHandler(void)
{
	if ((NRF_UART0->EVENTS_RXDRDY == 1) && (NRF_UART0->INTENSET & UART_INTENSET_RXDRDY_Msk)) 
    {
        NRF_UART0->EVENTS_RXDRDY = 0;
        uartRxBuffer[rxBufferSize] = NRF_UART0->RXD;
        rxBufferSize++;
        if(rxBufferSize >= HEADER_SIZE) {
            nrf_gpio_pin_toggle(LED_4);
            uint16_t size = (uartRxBuffer[1] << 8) + uartRxBuffer[2];
            if(rxBufferSize == size + HEADER_SIZE) {
                MainHeader_t mainHeader = (MainHeader_t)uartRxBuffer[0];
                switch (mainHeader)
                {
                    case GET:
                    {
                        ValueHeader_t valueHeader = (ValueHeader_t)uartRxBuffer[3];
                        switch (valueHeader) {
                            case FIRMWARE_VERSION:
                            {
                                char * fwVersion = "0.0.1";
                                unsigned char data[] = {ACK, 0, strlen(fwVersion)};
                                send_array(data, sizeof(data));
                                send_array((unsigned char *)fwVersion,strlen(fwVersion));
                                break;
                            }
                            case HARDWARE_VERSION:
                            {
                                char * hwVersion = "SMARTNODE1";
                                unsigned char data[] = {ACK, 0, strlen(hwVersion)};
                                send_array(data, sizeof(data));
                                send_array((unsigned char *)hwVersion,strlen(hwVersion));
                                break;
                            }
                            case SERIAL_NUMBER:
                            {
                                char serialNumber[6];
                                for(int i = 0; i < 4; i++)
                                    serialNumber[i] = NRF_FICR->DEVICEADDR[0] >> (i*8);
                                for(int i = 0; i < 2 ; i++)
                                    serialNumber[4 + i] = NRF_FICR->DEVICEADDR[1] >> (i*8);
                                
                                char buffer[128];
                                sprintf(buffer,"%02X:%02X:%02X:%02X:%02X:%02X", 
                                                serialNumber[5], serialNumber[4], serialNumber[3],
                                                serialNumber[2], serialNumber[1], serialNumber[0]);
                                unsigned char data[] = {ACK, 0, strlen(buffer)};
                                send_array(data, sizeof(data));
                                send_array((unsigned char *)buffer, strlen(buffer));
                                break;
                            }
                            case HUB_ENABLED:
                            {
                                break;
                            }
                            case HUB_JOINABLE:
                            {
                                break;
                            }
                        }
                        break;
                    }
                    case SET:
                    {
                        break;
                    }
                    case GET_NODE_VALUE:
                    {
                        break;
                    }
                    case SET_NODE_VALUE:
                    {
                        break;
                    }
                    default:
                        break;
                }
                rxBufferSize = 0;//Reset receive buffer
            }
        }
    }
}

void uart_init(void)
{
      nrf_gpio_cfg_output(TX_PIN_NUMBER);
      nrf_gpio_cfg_input(RX_PIN_NUMBER, NRF_GPIO_PIN_NOPULL);
      nrf_gpio_cfg_output(RTS_PIN_NUMBER);
      nrf_gpio_cfg_input(CTS_PIN_NUMBER, NRF_GPIO_PIN_NOPULL);  
      NRF_UART0->PSELTXD          = TX_PIN_NUMBER;
      NRF_UART0->PSELRXD          = RX_PIN_NUMBER;
      NRF_UART0->PSELCTS          = CTS_PIN_NUMBER;
      NRF_UART0->PSELRTS          = RTS_PIN_NUMBER;
      NRF_UART0->CONFIG           = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
      NRF_UART0->BAUDRATE         = (UART_BAUDRATE_BAUDRATE_Baud38400 << UART_BAUDRATE_BAUDRATE_Pos);
      NRF_UART0->ENABLE           = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);

      NVIC_EnableIRQ(UART0_IRQn);
      NRF_UART0->INTENSET					=  (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos);
      NRF_UART0->TASKS_STARTTX    = 1;
      NRF_UART0->TASKS_STARTRX    = 1;
      NRF_UART0->EVENTS_RXDRDY    = 0;
}

void send_byte(char c)
{
    NRF_UART0->TXD = (uint8_t)c;
    while (NRF_UART0->EVENTS_TXDRDY!=1) {}
    NRF_UART0->EVENTS_TXDRDY = 0;
}

void send_array(uint8_t * array, uint16_t size)
{
	for(uint16_t i = 0; i < size; i++) 
	{
		send_byte(array[i]);
	}
}
