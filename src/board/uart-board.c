#include <stdbool.h>
#include "stm32l0xx.h"
#include "utilities.h"
#include "board.h"
#include "gpio.h"
#include "gpio-board.h"
#include "uart-board.h"

#define UART_TX_FIFO_SIZE 1024
#define UART_RX_FIFO_SIZE 1024

static Uart_t *s_Uart2Obj = NULL;

static void EnableUsart2Clock(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    __DSB();
}

static void DisableUsart2Clock(void)
{
    RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN;
    __DSB();
}

static void ConfigureGpioPins(Uart_t *obj, PinNames tx, PinNames rx)
{
    GpioInit(&obj->Tx, tx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF4_USART2);
    GpioInit(&obj->Rx, rx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF4_USART2);
}

static void SetBaudrate(uint32_t baudrate)
{
    uint32_t clock = SystemCoreClock;
    uint32_t brr = (clock + (baudrate / 2U)) / baudrate;
    USART2->BRR = brr;
}

void UartMcuInit(Uart_t *obj, UartId_t uartId, PinNames tx, PinNames rx)
{
    if (obj == NULL)
    {
        return;
    }

    obj->UartId = uartId;
    obj->IsInitialized = true;

    if (uartId != UART_2)
    {
        return;
    }

    EnableUsart2Clock();
    ConfigureGpioPins(obj, tx, rx);
    s_Uart2Obj = obj;
}

void UartMcuConfig(Uart_t *obj, UartMode_t mode, uint32_t baudrate, WordLength_t wordLength, StopBits_t stopBits, Parity_t parity, FlowCtrl_t flowCtrl)
{
    (void)flowCtrl;

    if ((obj == NULL) || (obj->UartId != UART_2))
    {
        return;
    }

    // Only RX_TX, 8N1 supported
    if (mode != RX_TX || wordLength != UART_8_BIT || stopBits != UART_1_STOP_BIT || parity != NO_PARITY)
    {
        return;
    }

    EnableUsart2Clock();

    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;

    SetBaudrate(baudrate);

    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;
    USART2->CR1 |= USART_CR1_RXNEIE;

    NVIC_SetPriority(USART2_IRQn, 2);
    NVIC_EnableIRQ(USART2_IRQn);

    USART2->CR1 |= USART_CR1_UE;
}

void UartMcuDeInit(Uart_t *obj)
{
    if ((obj == NULL) || (obj->UartId != UART_2))
    {
        return;
    }

    USART2->CR1 &= ~(USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE);
    NVIC_DisableIRQ(USART2_IRQn);
    DisableUsart2Clock();
    s_Uart2Obj = NULL;
}

static void UartEnableTxInterrupt(void)
{
    USART2->CR1 |= USART_CR1_TXEIE;
}

static void UartDisableTxInterrupt(void)
{
    USART2->CR1 &= ~USART_CR1_TXEIE;
}

uint8_t UartMcuPutChar(Uart_t *obj, uint8_t data)
{
    if ((obj == NULL) || (obj->UartId != UART_2))
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

    bool txIdle = (USART2->ISR & USART_ISR_TXE) != 0U && (USART2->CR1 & USART_CR1_TXEIE) == 0U;

    if (txIdle && IsFifoEmpty(&obj->FifoTx))
    {
        USART2->TDR = data;
    }
    else
    {
        FifoPush(&obj->FifoTx, data);
        UartEnableTxInterrupt();
    }

    BoardCriticalSectionEnd(&mask);
    return 0;
}

uint8_t UartMcuGetChar(Uart_t *obj, uint8_t *data)
{
    if ((obj == NULL) || data == NULL || (obj->UartId != UART_2))
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
    if ((obj == NULL) || buffer == NULL)
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
    if ((obj == NULL) || buffer == NULL || nbReadBytes == NULL)
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

void USART2_IRQHandler(void)
{
    if (s_Uart2Obj == NULL)
    {
        return;
    }

    uint32_t isr = USART2->ISR;

    if (isr & USART_ISR_RXNE)
    {
        uint8_t byte = (uint8_t)(USART2->RDR & 0xFFU);
        if (!IsFifoFull(&s_Uart2Obj->FifoRx))
        {
            FifoPush(&s_Uart2Obj->FifoRx, byte);
            if (s_Uart2Obj->IrqNotify != NULL)
            {
                s_Uart2Obj->IrqNotify(UART_NOTIFY_RX);
            }
        }
    }

    if ((isr & USART_ISR_TXE) && (USART2->CR1 & USART_CR1_TXEIE))
    {
        if (!IsFifoEmpty(&s_Uart2Obj->FifoTx))
        {
            uint8_t byte = FifoPop(&s_Uart2Obj->FifoTx);
            USART2->TDR = byte;
        }
        else
        {
            UartDisableTxInterrupt();
            if (s_Uart2Obj->IrqNotify != NULL)
            {
                s_Uart2Obj->IrqNotify(UART_NOTIFY_TX);
            }
        }
    }

    if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE))
    {
        USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF;
    }
}
