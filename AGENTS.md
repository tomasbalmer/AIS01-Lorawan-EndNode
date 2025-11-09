# AIS01-LB Custom Firmware — Implementation Plan
*(Dragino STM32L072CZ + SX1276, AU915 Region)*

## 🧭 Project Overview

This project aims to develop a **custom firmware in C** for the **Dragino AIS01-LB** LoRaWAN AI Image End Node.  
The goal is to replace Dragino’s proprietary firmware while maintaining:
- Full LoRaWAN compatibility (AU915, OTAA, Class A)
- Support for Dragino-style AT commands
- Ultra-low power consumption
- A new feature for **remote calibration**

The firmware will be based on **LoRaMAC-node** (Semtech open-source stack) and compiled via **CLI using arm-none-eabi-gcc**, generating a `.bin` file to be flashed using the **Dragino OTA Tool**.

---

## ⚙️ Hardware Specification

| Component | Description |
|------------|--------------|
| Hardware: Dragino AIS01-LB End Node |
| MCU | STM32L072CZ (ARM Cortex-M0+) |
| Radio | Semtech SX1276 (SPI1 interface) |
| UART Interface | AT Commands / Debug |
| RTC | External LSE crystal, used for low power timing |
| Power Supply | 3.6 V Li-SOCl₂ battery |
| Bootloader | Dragino proprietary (preserve it, app offset: `0x08004000`) |
| LoRaWAN Region | AU915 – Sub-Band 2 (Channels 8–15) |
| LoRaWAN Class | Class A OTAA |
| Target Sleep Current | < 10 µA in STOP mode |

---

## 🧩 Firmware Architecture

```
/src
 ├── main.c               → Main state machine: BOOT→JOIN→TX/RX→SLEEP
 ├── lorawan_app.c/h      → LoRaMAC-node management, join, uplinks/downlinks
 ├── atcmd.c/h            → AT command parser + handler table
 ├── power.c/h            → STOP mode, RTC wake-up, peripheral power control
 ├── storage.c/h          → Flash persistence (configuration & keys)
 ├── board.c/h            → STM32–SX1276 pin mapping, UARTs, RTC setup
 ├── calibration.c/h      → New module: remote calibration (downlink/AT)
 ├── sensor.c/h           → Interface to AI sensor module (secondary UART)
 └── config.h             → Global defines, region, version, power configs
```

## 🧰 Required Tools

| Tool | Purpose |
|-------|----------|
| `arm-none-eabi-gcc` | CLI C compiler |
| `make` | Build automation |
| **Dragino OTA Tool** | UART flashing |
| **Git** | Version control |
| *(Optional)* STM32CubeProgrammer | SWD reflash / recovery |

---