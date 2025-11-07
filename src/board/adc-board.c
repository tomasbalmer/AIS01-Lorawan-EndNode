#include <stddef.h>
#include <stdbool.h>
#include "adc-board.h"
#include "gpio.h"

static ADC_HandleTypeDef AdcHandle;
static bool AdcClockReady = false;

static void AdcEnable(void)
{
    if ((ADC1->CR & ADC_CR_ADEN) == 0U)
    {
        ADC1->CR |= ADC_CR_ADEN;
        while ((ADC1->ISR & ADC_ISR_ADRDY) == 0U)
        {
        }
    }
}

static void AdcDisable(void)
{
    if ((ADC1->CR & ADC_CR_ADEN) != 0U)
    {
        ADC1->CR |= ADC_CR_ADDIS;
        while ((ADC1->CR & ADC_CR_ADEN) != 0U)
        {
        }
    }
}

void AdcMcuInit(Adc_t *obj, PinNames adcInput)
{
    if (obj == NULL || adcInput == NC)
    {
        return;
    }

    obj->Adc = ADC1;
    obj->AdcInput = adcInput;

    Gpio_t pin;
    GpioInit(&pin, adcInput, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0);

    __HAL_RCC_ADC1_CLK_ENABLE();
    AdcClockReady = true;

    AdcDisable();

    ADC1->CR |= ADC_CR_ADCAL;
    while ((ADC1->CR & ADC_CR_ADCAL) != 0U)
    {
    }

    ADC1->CFGR1 = 0U;
    ADC1->CFGR2 = 0U;
    ADC1->SMPR = ADC_SMPR_SMP;
    ADC1->CHSELR = 0U;
    ADC1->IER = 0U;

    AdcHandle.Instance = ADC1;
    AdcEnable();
}

void AdcMcuDeInit(Adc_t *obj)
{
    (void)obj;

    if (!AdcClockReady)
    {
        return;
    }

    AdcDisable();
    RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN;
    AdcClockReady = false;
}

uint16_t AdcMcuReadChannel(Adc_t *obj, uint32_t channel)
{
    if ((obj == NULL) || (obj->Adc != ADC1) || !AdcClockReady || (channel > 18U))
    {
        return 0;
    }

    AdcEnable();

    ADC1->CHSELR = 0U;
    ADC1->CHSELR = (1UL << channel);
    ADC1->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR | ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADSTART;

    while ((ADC1->ISR & ADC_ISR_EOC) == 0U)
    {
    }

    return (uint16_t)(ADC1->DR & 0xFFFFU);
}

void AdcMcuConfig(void)
{
    /* Configuration handled in AdcMcuInit */
}
