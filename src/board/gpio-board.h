#ifndef GPIO_BOARD_H
#define GPIO_BOARD_H

#include <stdint.h>
#include "gpio.h"

/* GPIO Alternate Function definitions for STM32L072 */
#define GPIO_AF0_SPI1 0x00U
#define GPIO_AF0_SPI2 0x00U
#define GPIO_AF1_SPI1 0x01U
#define GPIO_AF2_LPUART1 0x02U
#define GPIO_AF4_I2C1 0x04U
#define GPIO_AF4_USART1 0x04U
#define GPIO_AF4_USART2 0x04U
#define GPIO_AF5_I2C2 0x05U

void GpioMcuInit(Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value);
void GpioMcuSetContext(Gpio_t *obj, void *context);
void GpioMcuSetInterrupt(Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority, GpioIrqHandler *irqHandler);
void GpioMcuRemoveInterrupt(Gpio_t *obj);
void GpioMcuWrite(Gpio_t *obj, uint32_t value);
void GpioMcuToggle(Gpio_t *obj);
uint32_t GpioMcuRead(Gpio_t *obj);
void GpioMcuIrqHandler(uint8_t index);

#endif /* GPIO_BOARD_H */
