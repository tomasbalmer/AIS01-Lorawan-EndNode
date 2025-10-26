#ifndef UART_BOARD_H
#define UART_BOARD_H

#include "system/uart.h"

void UartMcuInit(Uart_t *obj, UartId_t uartId, PinNames tx, PinNames rx);
void UartMcuConfig(Uart_t *obj, UartMode_t mode, uint32_t baudrate, WordLength_t wordLength, StopBits_t stopBits, Parity_t parity, FlowCtrl_t flowCtrl);
void UartMcuDeInit(Uart_t *obj);
uint8_t UartMcuPutChar(Uart_t *obj, uint8_t data);
uint8_t UartMcuPutBuffer(Uart_t *obj, uint8_t *buffer, uint16_t size);
uint8_t UartMcuGetChar(Uart_t *obj, uint8_t *data);
uint8_t UartMcuGetBuffer(Uart_t *obj, uint8_t *buffer, uint16_t size, uint16_t *nbReadBytes);

#endif /* UART_BOARD_H */
