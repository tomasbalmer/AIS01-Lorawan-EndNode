#include "stm32l072xx.h"
#include "stm32l0xx.h"
#include "utilities.h"
#include "gpio-board.h"

static Gpio_t *s_GpioIrq[16];

static GPIO_TypeDef *PinToPort(PinNames pin)
{
    switch (pin & 0xF0)
    {
    case 0x00:
        return GPIOA;
    case 0x10:
        return GPIOB;
    case 0x20:
        return GPIOC;
    case 0x30:
        return GPIOD;
    case 0x40:
        return GPIOE;
    case 0x70:
        return GPIOH;
    default:
        return NULL;
    }
}

static uint32_t PinToClkMask(PinNames pin)
{
    switch (pin & 0xF0)
    {
    case 0x00:
        return RCC_IOPENR_IOPAEN;
    case 0x10:
        return RCC_IOPENR_IOPBEN;
    case 0x20:
        return RCC_IOPENR_IOPCEN;
    case 0x30:
        return RCC_IOPENR_IOPDEN;
    case 0x40:
        return RCC_IOPENR_IOPEEN;
    case 0x70:
        return RCC_IOPENR_IOPHEN;
    default:
        return 0;
    }
}

static uint8_t PinNumber(PinNames pin)
{
    return (uint8_t)(pin & 0x0F);
}

static uint8_t PinPortIndex(PinNames pin)
{
    return (uint8_t)((pin >> 4) & 0x07);
}

void GpioMcuInit(Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value)
{
    if (obj == NULL || pin == NC)
    {
        return;
    }

    GPIO_TypeDef *port = PinToPort(pin);
    if (port == NULL)
    {
        return;
    }

    uint32_t clkMask = PinToClkMask(pin);
    if (clkMask != 0)
    {
        RCC->IOPENR |= clkMask;
        __DSB();
    }

    uint8_t pinNum = PinNumber(pin);
    uint32_t pos = pinNum * 2U;

    obj->pin = pin;
    obj->pinIndex = (1U << pinNum);
    obj->port = port;
    obj->portIndex = PinPortIndex(pin);
    obj->pull = type;
    obj->Context = NULL;
    obj->IrqHandler = NULL;

    // Reset mode and pull configuration
    port->MODER &= ~(0x3U << pos);
    port->PUPDR &= ~(0x3U << pos);

    switch (mode)
    {
    case PIN_INPUT:
        port->MODER |= (0x0U << pos);
        break;
    case PIN_ANALOGIC:
        port->MODER |= (0x3U << pos);
        break;
    case PIN_ALTERNATE_FCT:
        port->MODER |= (0x2U << pos);
        break;
    case PIN_OUTPUT:
    default:
        port->MODER |= (0x1U << pos);
        break;
    }

    // Configure output type
    if (mode == PIN_OUTPUT || mode == PIN_ALTERNATE_FCT)
    {
        if (config == PIN_OPEN_DRAIN)
        {
            port->OTYPER |= obj->pinIndex;
        }
        else
        {
            port->OTYPER &= ~obj->pinIndex;
        }
        // Push speed to high for RF and timing sensitive pins
        port->OSPEEDR &= ~(0x3U << pos);
        port->OSPEEDR |= (0x3U << pos);
    }

    // Configure pull resistors
    switch (type)
    {
    case PIN_PULL_UP:
        port->PUPDR |= (0x1U << pos);
        break;
    case PIN_PULL_DOWN:
        port->PUPDR |= (0x2U << pos);
        break;
    case PIN_NO_PULL:
    default:
        // already cleared
        break;
    }

    if (mode == PIN_ALTERNATE_FCT)
    {
        uint8_t afrIndex = pinNum >> 3;
        uint8_t afrShift = (pinNum & 0x07) * 4U;
        uint32_t mask = (0xFU << afrShift);
        port->AFR[afrIndex] = (port->AFR[afrIndex] & ~mask) | ((value & 0xFU) << afrShift);
    }

    if (mode == PIN_OUTPUT)
    {
        if (value)
        {
            port->BSRR = obj->pinIndex;
        }
        else
        {
            port->BSRR = (uint32_t)obj->pinIndex << 16U;
        }
    }
}

void GpioMcuSetContext(Gpio_t *obj, void *context)
{
    if (obj != NULL)
    {
        obj->Context = context;
    }
}

void GpioMcuSetInterrupt(Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority, GpioIrqHandler *irqHandler)
{
    if (obj == NULL || obj->pin == NC)
    {
        return;
    }

    uint8_t pinNum = PinNumber(obj->pin);
    uint8_t portIndex = obj->portIndex;
    uint32_t line = obj->pinIndex;

    // Store IRQ handler
    s_GpioIrq[pinNum] = obj;
    obj->IrqHandler = irqHandler;

    // Configure SYSCFG EXTI line
    uint32_t extiCrIndex = pinNum >> 2;
    uint32_t extiCrShift = (pinNum & 0x03) * 4U;
    uint32_t mask = (0xFU << extiCrShift);

    SYSCFG->EXTICR[extiCrIndex] = (SYSCFG->EXTICR[extiCrIndex] & ~mask) | (portIndex << extiCrShift);

    // Configure trigger
    EXTI->RTSR &= ~line;
    EXTI->FTSR &= ~line;

    switch (irqMode)
    {
    case IRQ_RISING_EDGE:
        EXTI->RTSR |= line;
        break;
    case IRQ_FALLING_EDGE:
        EXTI->FTSR |= line;
        break;
    case IRQ_RISING_FALLING_EDGE:
        EXTI->RTSR |= line;
        EXTI->FTSR |= line;
        break;
    default:
        break;
    }

    // Clear pending interrupt
    EXTI->PR = line;

    // Enable interrupt
    EXTI->IMR |= line;

    // Configure NVIC
    IRQn_Type irqn;
    if (pinNum <= 1)
    {
        irqn = EXTI0_1_IRQn;
    }
    else if (pinNum <= 3)
    {
        irqn = EXTI2_3_IRQn;
    }
    else
    {
        irqn = EXTI4_15_IRQn;
    }

    uint32_t priority = (uint32_t)irqPriority;
    if (priority > 3U)
    {
        priority = 3U;
    }

    NVIC_SetPriority(irqn, priority);
    NVIC_EnableIRQ(irqn);
}

void GpioMcuRemoveInterrupt(Gpio_t *obj)
{
    if (obj == NULL)
    {
        return;
    }

    uint8_t pinNum = PinNumber(obj->pin);
    uint32_t line = obj->pinIndex;

    EXTI->IMR &= ~line;
    EXTI->RTSR &= ~line;
    EXTI->FTSR &= ~line;

    s_GpioIrq[pinNum] = NULL;
}

void GpioMcuWrite(Gpio_t *obj, uint32_t value)
{
    if ((obj == NULL) || (obj->port == NULL))
    {
        return;
    }

    GPIO_TypeDef *port = (GPIO_TypeDef *)obj->port;
    if (value)
    {
        port->BSRR = obj->pinIndex;
    }
    else
    {
        port->BSRR = (uint32_t)obj->pinIndex << 16U;
    }
}

void GpioMcuToggle(Gpio_t *obj)
{
    if ((obj == NULL) || (obj->port == NULL))
    {
        return;
    }

    GPIO_TypeDef *port = (GPIO_TypeDef *)obj->port;
    port->ODR ^= obj->pinIndex;
}

uint32_t GpioMcuRead(Gpio_t *obj)
{
    if ((obj == NULL) || (obj->port == NULL))
    {
        return 0;
    }

    GPIO_TypeDef *port = (GPIO_TypeDef *)obj->port;
    return (port->IDR & obj->pinIndex) ? 1U : 0U;
}

void GpioMcuIrqHandler(uint8_t index)
{
    if (index >= 16U)
    {
        return;
    }

    Gpio_t *gpio = s_GpioIrq[index];
    if ((gpio != NULL) && (gpio->IrqHandler != NULL))
    {
        gpio->IrqHandler(gpio->Context);
    }
}
