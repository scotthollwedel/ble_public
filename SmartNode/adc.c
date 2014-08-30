#include "adc.h"
#include "boards.h"
void adc_init(void) {
    NRF_ADC->CONFIG = (ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling <<ADC_CONFIG_INPSEL_Pos)
                      | (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos)
                      | (ADC_CONFIG_RES_8bit << ADC_CONFIG_RES_Pos);
    NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled << ADC_ENABLE_ENABLE_Pos;
}

float getBattVoltage(void)
{
    NRF_ADC->TASKS_START = 1;
    while(NRF_ADC->EVENTS_END == 0);
    return ((float)NRF_ADC->RESULT * 3.6/(float)255);
}
