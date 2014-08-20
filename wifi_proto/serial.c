#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf_gpio.h"
#include "serial_protocol.h"
#include "boards.h"
#include "serial.h"
#include "state_variables.h"

/* UART RX BUFFER HANDLER */
static uint8_t uartRxBuffer[100];
static uint8_t rxBufferSize = 0;

void UART0_IRQHandler(void)
{
	if ((NRF_UART0->EVENTS_RXDRDY == 1) &&
        (NRF_UART0->INTENSET & UART_INTENSET_RXDRDY_Msk)) {
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
                        case CHANGE_MODE:
                        {
                            ChangeMode_t changeMode = (ChangeMode_t)uartRxBuffer[3];
                            switch (changeMode)
                            {
                                case CHANGE_MODE_TRANSMITTER:
                                    setNewMode(TRANSMITTER_MODE);
                                    break;
                                case CHANGE_MODE_RECEIVER:
                                    setNewMode(RECEIVER_MODE);
                                    break;
                                case CHANGE_MODE_HUB:
                                    setNewMode(HUB_MODE);
                                    break;
                            }
                            send_array(uartRxBuffer, rxBufferSize);
                            break;
                        }
                        case GET_FIRMWARE_VERSION:
                        {
                            char * fwVersion = "0.0.1";
                            unsigned char data[] = {GET_FIRMWARE_VERSION, 0, strlen(fwVersion)};
                            send_array(data, sizeof(data));
                            send_array((unsigned char *)fwVersion,strlen(fwVersion));
                            break;
                        }
                        case GET_HARDWARE_VERSION:
                        {
                            char * hwVersion = "EZ1";
                            unsigned char data[] = {GET_HARDWARE_VERSION, 0, strlen(hwVersion)};
                            send_array(data, sizeof(data));
                            send_array((unsigned char *)hwVersion,strlen(hwVersion));
                            break;
                        }
                        case GET_SERIAL_NUMBER:
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
                            unsigned char data[] = {GET_SERIAL_NUMBER, 0, strlen(buffer)};
                            send_array(data, sizeof(data));
                            send_array((unsigned char *)buffer, strlen(buffer));
                            break;
                        }
                        case GET_BATTERY:
                            break;
                        case TRANSMITTER:
                        {
                            HandleTransmitter(&uartRxBuffer[3], size);
                            break;
                        }
                        case RECEIVER:
                        case HUB:
                        case LOCAL_UPGRADE:
                        case NAK:
                        case ACK:
                        default:
                            break;
                    }
                    rxBufferSize = 0;//Reset receive buffer
                }
            }
        }
}

void HandleTransmitter(unsigned char * buffer, uint8_t size)
{
    Transmitter_t transmitterSubheader = (Transmitter_t)buffer[0];
    switch (transmitterSubheader)
    {
        case SET_TRANSMITTER_VALUE:
        {
            TransmitterValues_t transmitterValue = (TransmitterValues_t)buffer[1];
            switch(transmitterValue)
            {
                case TRANSMITTER_PERIOD:
                {
                    setBLEBeaconTransmitPeriod(buffer[2]);
                    unsigned char data[] = {TRANSMITTER, 0, 3, SET_TRANSMITTER_VALUE, TRANSMITTER_PERIOD, getBLEBeaconTransmitPeriod()};
                    send_array(data, sizeof(data));
                    break;
                }
                case TRANSMITTER_OUTPUT_POWER:
                {
                    TransmitterOutputPower_t transmitterOutputPower = (TransmitterOutputPower_t)buffer[2];
                    switch(transmitterOutputPower)
                    {
                        case OUTPUT_POWER_0dB:
                            setBLEBeaconTransmitPower(RADIO_TXPOWER_TXPOWER_0dBm);
                            break;
                        case OUTPUT_POWER_Neg4dB:
                            setBLEBeaconTransmitPower(RADIO_TXPOWER_TXPOWER_Neg4dBm);
                            break;
                        case OUTPUT_POWER_Neg8dB:
                            setBLEBeaconTransmitPower(RADIO_TXPOWER_TXPOWER_Neg8dBm);
                            break;
                        case OUTPUT_POWER_Neg12dB:
                            setBLEBeaconTransmitPower(RADIO_TXPOWER_TXPOWER_Neg12dBm);
                            break;
                        case OUTPUT_POWER_Neg16dB:
                            setBLEBeaconTransmitPower(RADIO_TXPOWER_TXPOWER_Neg16dBm);
                            break;
                        case OUTPUT_POWER_Neg20dB:
                            setBLEBeaconTransmitPower(RADIO_TXPOWER_TXPOWER_Neg20dBm);
                            break;
                        case OUTPUT_POWER_Neg30dB:
                            setBLEBeaconTransmitPower(RADIO_TXPOWER_TXPOWER_Neg30dBm);
                            break;
                    }
                    unsigned char data[] = {TRANSMITTER, 0, 3, SET_TRANSMITTER_VALUE, TRANSMITTER_OUTPUT_POWER, buffer[2]};
                    send_array(data, sizeof(data));
                    break;
                }
                case TRANSMITTER_PAYLOAD:
                {
                    setBLEBeaconPayload(&buffer[2], size - 2);
                    unsigned char data[] = {TRANSMITTER, 0, 3};
                    send_array(data, sizeof(data));
                    send_array(buffer, size);
                    break;
                }
            }
            break;
        }
        case GET_TRANSMITTER_VALUE:
            break;
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
      //NRF_UART0->CONFIG           = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
      NRF_UART0->BAUDRATE         = (UART_BAUDRATE_BAUDRATE_Baud38400 << UART_BAUDRATE_BAUDRATE_Pos);
      NRF_UART0->ENABLE           = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);

      NVIC_EnableIRQ(UART0_IRQn);
      NRF_UART0->INTENSET					=  (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos);
      NRF_UART0->TASKS_STARTTX    = 1;
      //NRF_UART0->TASKS_STARTRX    = 1;
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
