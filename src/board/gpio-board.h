#ifndef GPIO_BOARD_H
#define GPIO_BOARD_H

#include <stdint.h>
#include "pinName-board.h"
#include "system/gpio.h"

void GpioMcuInit(Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value);
void GpioMcuSetContext(Gpio_t *obj, void *context);
void GpioMcuSetInterrupt(Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority, GpioIrqHandler *irqHandler);
void GpioMcuRemoveInterrupt(Gpio_t *obj);
void GpioMcuWrite(Gpio_t *obj, uint32_t value);
void GpioMcuToggle(Gpio_t *obj);
uint32_t GpioMcuRead(Gpio_t *obj);
void GpioMcuIrqHandler(uint8_t index);

#endif /* GPIO_BOARD_H */
