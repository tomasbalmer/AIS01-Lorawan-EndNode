# Dragino AIS01-LB Custom Firmware

Custom firmware for the Dragino AIS01-LB (STM32L072CZ + SX1276) replacing the OEM image while keeping LoRaWAN Class A OTAA compatibility for AU915 Sub-band 2. The codebase is built around Semtech's LoRaMAC-node stack with a new bare-metal board layer, AT command surface, and remote calibration engine.

## Features

- **LoRaWAN Region**: AU915 (Sub-band 2)
- **Class**: Class A (OTAA), periodic uplinks + confirmed/unconfirmed modes
- **Remote calibration**: AT command `AT+CALIBREMOTE` and downlink opcode `0xA0`
- **Low power**: STOP mode + RTC wake-up, SX1276 power gating (<20 µA target)
- **Storage**: EEPROM emulation with CRC32 validation for credentials and settings
- **Tooling**: GNU make + `arm-none-eabi-gcc`, outputs at application offset `0x0800F000`

## Quick Start

### 1. Build the firmware

```bash
make clean && make -j4
```

Artifacts are written to `build/`, notably `build/ais01.bin` (≈50 KB).

### 2. Flash the device

Use the Dragino OTA Tool and select `build/ais01.bin` (application offset `0x0800F000`).

### Flashing the firmware (updated for OEM-aligned memory layout)
The application offset has been aligned with the OEM firmware:

**Application Base Address:** `0x0800F000`  
**Bootloader OEM Region:**   `0x08000000 – 0x0800EFFF`  
**Vector Table:**            `0x0800F000`

Flash command:
```bash
st-flash write build/ais01.bin 0x0800F000
```

### 3. Open the serial console

```bash
minicom -D /dev/ttyUSB0 -b 115200
```

### 4. Provision LoRaWAN credentials

```text
AT+DEVEUI=<16-char hex>
AT+APPEUI=<16-char hex>
AT+APPKEY=<32-char hex>
AT+JOIN
```

## Documentation

- **Architecture overview** – `docs/rebuild/Firmware_Architecture_Map.md`
- **AT command handlers** – `docs/rebuild/AT_Handlers.md`
- **Downlink opcode map** – `docs/rebuild/Downlink_Dispatcher.md`
- **Remote calibration details** – `docs/rebuild/Calibration_Engine.md`
- **Power optimisation notes** – `docs/rebuild/Hardware_Power.md`
- **Validation plan** – `docs/rebuild/Test_Plan.md`

Historical reverse-engineering notes (Ghidra, OEM firmware) live under `docs/AIS01_bin_*` and `docs/firmware/`.

## Project Structure

```
AIS01-Lorawan-EndNode/
├── src/
│   ├── app/          # Application logic, AT surface, storage, power, calibration
│   ├── board/        # STM32L072 + SX1276 board support (GPIO, UART, SPI, RTC, LPM)
│   ├── cmsis/        # CMSIS headers, startup, system clock
│   ├── lorawan/      # Semtech LoRaMAC-node core, crypto, regions
│   ├── radio/        # SX1276 radio driver
│   └── system/       # Portable utilities (timers, fifo, nvmm, HAL stubs)
├── docs/             # Specs, RE notes, test plans
├── Makefile          # Build system (PROJECT=ais01)
└── stm32l072xx_flash_app.ld  # Linker script with 0x0800F000 app offset
```

## Requirements

- `arm-none-eabi-gcc` 10.3+ (tested with Arm GNU Toolchain 14.3 rel1)
- GNU Make
- Dragino OTA Tool (or SWD programmer)
- AU915 gateway with Sub-band 2 enabled

## Key AT Commands

| Command | Description | Example |
|---------|-------------|---------|
| `AT` | Ping the parser | `AT` → `OK` |
| `AT+VER` | Firmware version banner | `AT+VER` |
| `AT+DEVEUI=<hex>` | Set DevEUI | `AT+DEVEUI=0102030405060708` |
| `AT+APPEUI=<hex>` | Set AppEUI | `AT+APPEUI=0102030405060708` |
| `AT+APPKEY=<hex>` | Set AppKey | `AT+APPKEY=001122...` |
| `AT+JOIN` | Trigger OTAA join | `AT+JOIN` |
| `AT+TDC=<ms>` | Set TX interval | `AT+TDC=60000` |
| `AT+ADR=<0/1>` | Enable or disable ADR | `AT+ADR=1` |
| `AT+CALIBREMOTE=<hex>` | Remote calibration payload apply/query | `AT+CALIBREMOTE=0123...` |
| `ATZ` | Soft reboot | `ATZ` |

Consult `docs/rebuild/AT_Handlers.md` for the full list.

## Power Consumption Targets

| Mode | Current | Notes |
|------|---------|-------|
| TX (LoRa) | ~120 mA | Depends on RF output power |
| RUN | ~1.5 mA | MCU active, peripherals as needed |
| STOP | <20 µA | RTC + SRAM retention only |

## Versioning

Current development build: **1.0.0-dev** (custom firmware replacing Dragino OEM).

## Credits

Developed by Waterplan. LoRaMAC-node components © Semtech.
