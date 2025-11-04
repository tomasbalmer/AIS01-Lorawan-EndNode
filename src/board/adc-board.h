/*!
 * \file      adc-board.h
 *
 * \brief     Target board ADC driver definitions
 */
#ifndef __ADC_BOARD_H__
#define __ADC_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32l0xx.h"
#include "gpio.h"

/*!
 * ADC object type definition
 */
typedef struct Adc_s
{
    ADC_TypeDef *Adc;
    PinNames AdcInput;
} Adc_t;

/*!
 * ADC channel configuration structure
 */
typedef struct
{
    uint32_t Channel;
    uint32_t Rank;
    uint32_t SamplingTime;
    uint32_t SingleDiff;
    uint32_t OffsetNumber;
    uint32_t Offset;
} ADC_ChannelConfTypeDef;

/*!
 * Simplified ADC handle structure for bare-metal use
 */
typedef struct
{
    ADC_TypeDef *Instance;
    struct {
        uint32_t OversamplingMode;
        uint32_t ClockPrescaler;
        uint32_t Resolution;
        uint32_t SamplingTime;
        uint32_t ScanConvMode;
        uint32_t DataAlign;
        uint32_t ContinuousConvMode;
        uint32_t DiscontinuousConvMode;
        uint32_t ExternalTrigConv;
        uint32_t ExternalTrigConvEdge;
        uint32_t DMAContinuousRequests;
        uint32_t EOCSelection;
        uint32_t LowPowerAutoWait;
        uint32_t LowPowerAutoPowerOff;
        uint32_t LowPowerFrequencyMode;
        uint32_t Overrun;
    } Init;
} ADC_HandleTypeDef;

/* ADC Configuration defines */
#define DISABLE                         0
#define ENABLE                          1
#define ADC_CLOCK_SYNC_PCLK_DIV2        0x00000000U
#define ADC_RESOLUTION_12B              0x00000000U
#define ADC_SAMPLETIME_160CYCLES_5      0x00000007U
#define ADC_SCAN_DIRECTION_FORWARD      0x00000000U
#define ADC_DATAALIGN_RIGHT             0x00000000U
#define ADC_EXTERNALTRIGCONV_T6_TRGO    0x00000000U
#define ADC_EXTERNALTRIGCONVEDGE_NONE   0x00000000U
#define ADC_EOC_SEQ_CONV                0x00000000U
#define ADC_EOC_SINGLE_CONV             0x00000001U
#define ADC_OVR_DATA_PRESERVED          0x00000000U
#define ADC_SOFTWARE_START              0x00000000U
#define ADC_SINGLE_ENDED                0x00000000U
#define ADC_CHANNEL_MASK                0x0000001FU
#define ADC_RANK_CHANNEL_NUMBER         0x00000001U
#define ADC_RANK_NONE                   0x00000000U

/* HAL status enum */
typedef enum
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

#define HAL_MAX_DELAY               0xFFFFFFFFU

/* HAL macros */
#define RESET                           0
#define SET                             1
#define RCC_FLAG_HSIRDY                 RCC_CR_HSIRDY
#define PWR_FLAG_VREFINTRDY             PWR_CSR_VREFINTRDYF
#define __HAL_RCC_HSI_ENABLE()          (RCC->CR |= RCC_CR_HSION)
#define __HAL_RCC_GET_FLAG(FLAG)        ((RCC->CR & (FLAG)) != 0U ? SET : RESET)
#define __HAL_PWR_GET_FLAG(FLAG)        ((PWR->CSR & (FLAG)) != 0U ? SET : RESET)
#define __HAL_RCC_ADC1_CLK_ENABLE()     (RCC->APB2ENR |= RCC_APB2ENR_ADC1EN)

/* HAL function stubs */
static inline HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef* hadc) { (void)hadc; return HAL_OK; }
static inline HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* hadc) { (void)hadc; return HAL_OK; }

/*!
 * \brief Initializes the ADC
 *
 * \param [IN] obj ADC object
 * \param [IN] adcInput ADC input pin
 */
void AdcMcuInit( Adc_t *obj, PinNames adcInput );

/*!
 * \brief DeInitializes the ADC
 *
 * \param [IN] obj ADC object
 */
void AdcMcuDeInit( Adc_t *obj );

/*!
 * \brief Read the analog value
 *
 * \param [IN] obj ADC object
 * \param [IN] channel ADC channel to read
 * \retval value Analog value
 */
uint16_t AdcMcuReadChannel( Adc_t *obj, uint32_t channel );
void AdcMcuConfig( void );

#ifdef __cplusplus
}
#endif

#endif /* __ADC_BOARD_H__ */
