#include <stddef.h>
#include "adc-board.h"

static ADC_HandleTypeDef AdcHandle;

void AdcMcuInit(Adc_t *obj, PinNames adcInput)
{
    if (obj == NULL)
    {
        return;
    }

    obj->Adc = NULL;
    obj->AdcInput = adcInput;
    (void)AdcHandle;
}

void AdcMcuDeInit(Adc_t *obj)
{
    (void)obj;
}

uint16_t AdcMcuReadChannel(Adc_t *obj, uint32_t channel)
{
    (void)obj;
    (void)channel;
    /* Stub: return a fixed value; customize with actual ADC implementation */
    return 0;
}

void AdcMcuConfig(void)
{
    /* Stub: no additional hardware configuration required */
}

