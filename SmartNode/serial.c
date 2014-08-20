#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "serial.h"

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
      NRF_UART0->BAUDRATE         = (UART_BAUDRATE_BAUDRATE_Baud9600 << UART_BAUDRATE_BAUDRATE_Pos);
      NRF_UART0->ENABLE           = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);

      NVIC_EnableIRQ(UART0_IRQn);
      NRF_UART0->INTENSET					=  (UART_INTENSET_RXDRDY_Set << UART_INTENSET_RXDRDY_Pos);
      NRF_UART0->TASKS_STARTTX    = 1;
      NRF_UART0->TASKS_STARTRX    = 1;
      NRF_UART0->EVENTS_RXDRDY    = 0;
}

void debug_print(const char * str)
{
    puts(str);
    puts("\r\n");
}
void send_byte(char c)
{
    NRF_UART0->TXD = (uint8_t)c;
    while (NRF_UART0->EVENTS_TXDRDY!=1) {}
    NRF_UART0->EVENTS_TXDRDY = 0;
}

int puts(const char * __str) {
    int i = 0;
    while(__str[i]) {
        send_byte(__str[i]);
        i++;
    }
    return 1;
}
