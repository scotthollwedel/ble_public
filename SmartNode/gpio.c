#include <stdint.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "spi_master_config.h"
#include "spi.h"
#include "nrf_delay.h"
#include "serial.h"

void gpio_init(void)
{
	nrf_gpio_range_cfg_output(LED_START, LED_STOP);
    nrf_gpio_cfg_input(BUTTON_0, BUTTON_PULL);
   // nrf_gpio_cfg_input(SPI_PSELIRQ, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_cfg_input(SPI_PSELIRQ, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_pin_clear(VBAT_SW_EN);
    nrf_gpio_cfg_output(VBAT_SW_EN);
    // Enable interrupt:
    NVIC_EnableIRQ(GPIOTE_IRQn);
	NRF_GPIOTE->CONFIG[0] =  (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos)
                           | (SPI_PSELIRQ << GPIOTE_CONFIG_PSEL_Pos)  
                           | (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos);
}

void gpio_power_up_cc3000(void) {
    nrf_gpio_pin_set(VBAT_SW_EN);
}

void gpio_power_down_cc3000(void) {
    nrf_gpio_pin_clear(VBAT_SW_EN);
}

void gpio_enable_cc3000_irq(void)
{
    NRF_GPIOTE->INTENSET  = (GPIOTE_INTENSET_IN0_Set << GPIOTE_INTENSET_IN0_Pos); 
}

void gpio_disable_cc3000_irq(void)
{
    NRF_GPIOTE->INTENCLR  = (GPIOTE_INTENSET_IN0_Set << GPIOTE_INTENSET_IN0_Pos); 
}

int gpio_get_cc3000_irq(void)
{
    return nrf_gpio_pin_read(SPI_PSELIRQ);
}

void GPIOTE_IRQHandler(void)
{
    // Event causing the interrupt must be cleared.
    if ((NRF_GPIOTE->EVENTS_IN[0] == 1) && 
        (NRF_GPIOTE->INTENSET & GPIOTE_INTENSET_IN0_Msk))
    {
        NRF_GPIOTE->EVENTS_IN[0] = 0;
        IntSpiGPIOHandler();
    }
}
