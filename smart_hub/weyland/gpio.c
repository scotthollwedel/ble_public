#include <stdint.h>
#include "nrf_gpio.h"
#include "boards.h"

void gpio_init(void)
{
	nrf_gpio_range_cfg_output(LED_START, LED_STOP);
    nrf_gpio_cfg_input(BUTTON_0, BUTTON_PULL);
    // Enable interrupt:
    NVIC_EnableIRQ(GPIOTE_IRQn);
    NRF_GPIOTE->CONFIG[0] =  (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos)
                           | (0 << GPIOTE_CONFIG_PSEL_Pos)  
                           | (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos);
	NRF_GPIOTE->CONFIG[1] =  (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos)
                           | (1 << GPIOTE_CONFIG_PSEL_Pos)  
                           | (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos);
    NRF_GPIOTE->INTENSET  = (GPIOTE_INTENSET_IN0_Set << GPIOTE_INTENSET_IN0_Pos) 
	                         | (GPIOTE_INTENSET_IN1_Set << GPIOTE_INTENSET_IN1_Pos);
}

void GPIOTE_IRQHandler(void)
{
    // Event causing the interrupt must be cleared.
    if ((NRF_GPIOTE->EVENTS_IN[0] == 1) && 
        (NRF_GPIOTE->INTENSET & GPIOTE_INTENSET_IN0_Msk))
    {
    }
	if ((NRF_GPIOTE->EVENTS_IN[1] == 1) && 
        (NRF_GPIOTE->INTENSET & GPIOTE_INTENSET_IN1_Msk))
    {
    }
}
