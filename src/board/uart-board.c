#include <stdbool.h>
#include "stm32l0xx.h"
#include "utilities.h"
#include "board.h"
#include "gpio.h"
#include "gpio-board.h"
#include "uart-board.h"

#define UART_INSTANCE_COUNT 2

static Uart_t *s_UartObjects[UART_INSTANCE_COUNT] = { NULL, NULL };

static int8_t UartIndex(UartId_t id)
{
    switch (id)
    {
    case UART_1:
        return 0;
    case UART_2:
        return 1;
    default:
        return -1;
    }
}

static USART_TypeDef *GetUsartInstance(UartId_t id)
{
    switch (id)
    {
    case UART_1:
        return USART1;
    case UART_2:
        return USART2;
    default:
        return NULL;
    }
}

static IRQn_Type GetUsartIrq(UartId_t id)
{
    switch (id)
    {
    case UART_1:
        return USART1_IRQn;
    case UART_2:
        return USART2_IRQn;
    default:
        return (IRQn_Type)0;
    }
}

static void EnableUsartClock(UartId_t id)
{
    switch (id)
    {
    case UART_1:
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        break;
    case UART_2:
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
        break;
    default:
        return;
    }
    __DSB();
}

static void DisableUsartClock(UartId_t id)
{
    switch (id)
    {
    case UART_1:
        RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
        break;
    case UART_2:
        RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN;
        break;
    default:
        return;
    }
    __DSB();
}

static uint32_t GetAlternateFunction(UartId_t id)
{
    return (id == UART_1) ? GPIO_AF4_USART1 : GPIO_AF4_USART2;
}

static void ConfigureGpioPins(Uart_t *obj, PinNames tx, PinNames rx, UartId_t id)
{
    uint32_t af = GetAlternateFunction(id);
    GpioInit(&obj->Tx, tx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, af);
    GpioInit(&obj->Rx, rx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, af);
}

static void SetBaudrate(USART_TypeDef *instance, uint32_t baudrate)
{
    uint32_t clock = SystemCoreClock;
    uint32_t brr = (clock + (baudrate / 2U)) / baudrate;
    instance->BRR = brr;
}

static void UartEnableTxInterrupt(USART_TypeDef *instance)
{
    instance->CR1 |= USART_CR1_TXEIE;
}

static void UartDisableTxInterrupt(USART_TypeDef *instance)
{
    instance->CR1 &= ~USART_CR1_TXEIE;
}

void UartMcuInit(Uart_t *obj, UartId_t uartId, PinNames tx, PinNames rx)
{
    if (obj == NULL)
    {
        return;
    }

    int8_t index = UartIndex(uartId);
    if (index < 0)
    {
        return;
    }

    obj->UartId = uartId;
    obj->IsInitialized = true;

    EnableUsartClock(uartId);
    ConfigureGpioPins(obj, tx, rx, uartId);
    s_UartObjects[index] = obj;
}

void UartMcuConfig(Uart_t *obj, UartMode_t mode, uint32_t baudrate, WordLength_t wordLength, StopBits_t stopBits, Parity_t parity, FlowCtrl_t flowCtrl)
{
    (void)flowCtrl;

    if ((obj == NULL) || (mode != RX_TX) || (wordLength != UART_8_BIT) ||
        (stopBits != UART_1_STOP_BIT) || (parity != NO_PARITY))
    {
        return;
    }

    USART_TypeDef *instance = GetUsartInstance(obj->UartId);
    if (instance == NULL)
    {
        return;
    }

    EnableUsartClock(obj->UartId);

    instance->CR1 = 0;
    instance->CR2 = 0;
    instance->CR3 = 0;

    SetBaudrate(instance, baudrate);

    instance->CR1 |= USART_CR1_TE | USART_CR1_RE;
    instance->CR1 |= USART_CR1_RXNEIE;

    IRQn_Type irq = GetUsartIrq(obj->UartId);
    NVIC_SetPriority(irq, 2);
    NVIC_EnableIRQ(irq);

    instance->CR1 |= USART_CR1_UE;
}

void UartMcuDeInit(Uart_t *obj)
{
    if (obj == NULL)
    {
        return;
    }

    USART_TypeDef *instance = GetUsartInstance(obj->UartId);
    if (instance == NULL)
    {
        return;
    }

    instance->CR1 &= ~(USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_TXEIE);
    NVIC_DisableIRQ(GetUsartIrq(obj->UartId));
    DisableUsartClock(obj->UartId);

    int8_t index = UartIndex(obj->UartId);
    if (index >= 0)
    {
        s_UartObjects[index] = NULL;
    }
}

uint8_t UartMcuPutChar(Uart_t *obj, uint8_t data)
{
    if (obj == NULL)
    {
        return 1;
    }

    USART_TypeDef *instance = GetUsartInstance(obj->UartId);
    if (instance == NULL)
    {
        return 1;
    }

    uint32_t mask;
    BoardCriticalSectionBegin(&mask);

    if (IsFifoFull(&obj->FifoTx))
    {
        BoardCriticalSectionEnd(&mask);
        return 1;
    }

    bool txIdle = ((instance->ISR & USART_ISR_TXE) != 0U) && ((instance->CR1 & USART_CR1_TXEIE) == 0U);

    if (txIdle && IsFifoEmpty(&obj->FifoTx))
    {
        instance->TDR = data;
    }
    else
    {
        FifoPush(&obj->FifoTx, data);
        UartEnableTxInterrupt(instance);
    }

    BoardCriticalSectionEnd(&mask);
    return 0;
}

uint8_t UartMcuGetChar(Uart_t *obj, uint8_t *data)
{
    if ((obj == NULL) || (data == NULL))
    {
        return 1;
    }

    uint32_t mask;
    BoardCriticalSectionBegin(&mask);

    if (IsFifoEmpty(&obj->FifoRx))
    {
        BoardCriticalSectionEnd(&mask);
        return 1;
    }

    *data = FifoPop(&obj->FifoRx);
    BoardCriticalSectionEnd(&mask);
    return 0;
}

uint8_t UartMcuPutBuffer(Uart_t *obj, uint8_t *buffer, uint16_t size)
{
    if ((obj == NULL) || (buffer == NULL))
    {
        return 1;
    }

    for (uint16_t i = 0; i < size; i++)
    {
        if (UartMcuPutChar(obj, buffer[i]) != 0U)
        {
            return 1;
        }
    }
    return 0;
}

uint8_t UartMcuGetBuffer(Uart_t *obj, uint8_t *buffer, uint16_t size, uint16_t *nbReadBytes)
{
    if ((obj == NULL) || (buffer == NULL) || (nbReadBytes == NULL))
    {
        return 1;
    }

    *nbReadBytes = 0;
    for (uint16_t i = 0; i < size; i++)
    {
        if (UartMcuGetChar(obj, &buffer[i]) != 0U)
        {
            break;
        }
        (*nbReadBytes)++;
    }
    return 0;
}

static void UartHandleIrq(UartId_t id)
{
    int8_t index = UartIndex(id);
    if (index < 0)
    {
        return;
    }

    Uart_t *obj = s_UartObjects[index];
    USART_TypeDef *instance = GetUsartInstance(id);
    if ((obj == NULL) || (instance == NULL))
    {
        return;
    }

    uint32_t isr = instance->ISR;

    if (isr & USART_ISR_RXNE)
    {
        uint8_t byte = (uint8_t)(instance->RDR & 0xFFU);
        if (!IsFifoFull(&obj->FifoRx))
        {
            FifoPush(&obj->FifoRx, byte);
            if (obj->IrqNotify != NULL)
            {
                obj->IrqNotify(UART_NOTIFY_RX);
            }
        }
    }

    if ((isr & USART_ISR_TXE) && (instance->CR1 & USART_CR1_TXEIE))
    {
        if (!IsFifoEmpty(&obj->FifoTx))
        {
            uint8_t byte = FifoPop(&obj->FifoTx);
            instance->TDR = byte;
        }
        else
        {
            UartDisableTxInterrupt(instance);
            if (obj->IrqNotify != NULL)
            {
                obj->IrqNotify(UART_NOTIFY_TX);
            }
        }
    }

    if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE))
    {
        instance->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF;
    }
}

void USART1_IRQHandler(void)
{
    UartHandleIrq(UART_1);
}

void USART2_IRQHandler(void)
{
    UartHandleIrq(UART_2);
}
