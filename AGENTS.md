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

---

## 🧠 AT Commands to Implement

### LoRaWAN Configuration
| Command | Description |
|----------|--------------|
| `AT+DEVEUI` | Set / Read DevEUI |
| `AT+APPEUI` | Set / Read AppEUI |
| `AT+APPKEY` | Set / Read AppKey |
| `AT+NJM` | Join mode (0=ABP, 1=OTAA) |
| `AT+JOIN` | Trigger OTAA Join |
| `AT+ADR` | Enable/Disable ADR |
| `AT+DR` | Set Data Rate |
| `AT+TXP` | Set TX power |
| `AT+RX2DR` | Set RX2 Data Rate |
| `AT+RX2FQ` | Set RX2 Frequency |
| `AT+RX1DL` / `AT+RX2DL` | RX window delays |
| `AT+CHE` | Sub-band select |
| `AT+FREQBAND` | Region band setting |

### Application & Uplink
| Command | Description |
|----------|--------------|
| `AT+TDC` | Transmission interval (ms) |
| `AT+PORT` | FPort for uplinks |
| `AT+PNACKMD` | Confirmed / Unconfirmed mode |
| `AT+UPTM` | Trigger manual uplink |

### System
| Command | Description |
|----------|--------------|
| `AT+CFG` | Print current configuration |
| `AT+VER` | Firmware version |
| `AT+BAT` | Battery voltage |
| `AT+FDR` | Factory Reset |
| `ATZ` | Reboot |
| `AT+DEBUG` | Enable/disable UART debug output |

### Custom (New)
| Command | Description |
|----------|--------------|
| `AT+CALIBREMOTE=<hex>` | Perform remote sensor calibration |

---

## 🛰️ Downlink Commands

| Opcode | Description |
|---------|--------------|
| `0x01` | Set TDC |
| `0x21` | Set ADR |
| `0x22` | Set DR |
| `0x23` | Set TX Power |
| `0x26` | Request Status (uplink response) |
| `0xA0` | Remote Calibration (new feature) |

---

## 🔋 Power Strategy

- Use **STOP Mode + RTC Wake-up**
- Power down **SX1276** outside TX/RX windows
- Disable UARTs, ADC, and SPI when idle
- Avoid blocking delays (use timers / interrupts)
- Target sleep current: < 20 µA

---

## 🧰 Required Tools

| Tool | Purpose |
|-------|----------|
| `arm-none-eabi-gcc` | CLI C compiler |
| `make` | Build automation |
| **Dragino OTA Tool** | UART flashing |
| **Git** | Version control |
| *(Optional)* STM32CubeProgrammer | SWD reflash / recovery |

---

## 🔧 Implementation Roadmap

### Phase 1 — Setup
- Clone **LoRaMAC-node** repository.
- Create project folder: `projects/AIS01-LB/`.
- Add Makefile and linker script (`0x08004000` offset).
- Compile dummy app and generate `.bin`.

### Phase 2 — Board Bring-Up
- Define SX1276 pins (NSS, RESET, DIO0–2).
- Configure SPI1, UART_AT (115200), RTC with LSE.
- Initialize HAL and minimal peripherals.

### Phase 3 — LoRaMAC Integration
- Implement `lorawan_app.c`: AU915 OTAA join, dummy uplink, callbacks (confirm/indication).
- Verify join success through Dragino gateway + AWS IoT Core.

### Phase 4 — AT Command Layer
- Implement `atcmd.c`: parser and command table.
- Add initial handlers: JOIN, TDC, ADR, DR, CFG, BAT, VER.
- Standard responses: `OK` / `ERROR`.

### Phase 5 — Storage
- Implement `storage.c`: EEPROM emulation for config persistence.
- `AT+FDR` → erase flash + restore defaults.
- Load config at boot.

### Phase 6 — Downlink & Remote Calibration
- Downlink opcode dispatcher inside RX1/RX2 callbacks.
- Implement opcode `0xA0` → `calibration_apply()`.
- Send confirmation uplink after applying calibration.

### Phase 7 — Power Optimization
- Enter STOP mode between TX/RX events.
- Suspend all peripherals and SX1276 during idle.
- Measure and confirm standby current < 20 µA.

### Phase 8 — QA & Validation
- Test commands:  
  `AT+JOIN`, `AT+TDC=60000`, `AT+ADR=1`, `AT+CALIBREMOTE=...`
- Validate flash persistence after reset.
- Confirm uplink payloads in AWS IoT Core.

---

## 🔬 Documentation References

- AIS01-LB User Manual – DRAGINO
- End Device AT Commands & Downlink Command – DRAGINO
- Battery Life Guide – DRAGINO
- LoRaWAN Communication Debug Guide – DRAGINO
- AI Image Sensor Calibration – DRAGINO
- OTA Update Firmware for AI Sensors – DRAGINO
- Setup Gateway with AWS IoT Core – DRAGINO
- DLOS8 Gateway Installation Manual – DRAGINO

---

## ✅ Deliverables

1. Complete source code skeleton (`.c/.h`) with function stubs.
2. Fully functional AT command parser.
3. LoRaWAN Join (OTAA) + periodic uplink.
4. Downlink dispatcher with `CALIBREMOTE` handler.
5. CLI-ready Makefile and `.ld` linker script.
6. This `.md` document maintained as living design spec.

---

## 🧩 Instruction to Embedded AI

> Using this Implementation Plan as context, generate or improve the firmware modules for the AIS01-LB.
> Follow the LoRaMAC-node architecture, optimize for power efficiency, and ensure AU915 compatibility.
> Implement and document all listed AT commands and maintain clean, modular, well-commented code.
