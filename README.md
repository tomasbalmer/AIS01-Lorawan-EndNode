# AIS01-LB — Custom LoRaWAN Firmware (OEM-Compatible Layout)

Custom firmware for the **Dragino AIS01-LB** (STM32L072CZ + SX1276), implementing a clean, modern LoRaWAN Class A end-node using Semtech’s LoRaMAC-node stack and a fully custom application layer.  
The OEM reverse-engineered behavior was used **only as inspiration**; the firmware is now fully self-consistent and maintained independently.

This README is the **source of truth** for building, flashing, and using the firmware.

---

# ✨ Key Features

- **LoRaWAN Region:** AU915 (Sub-band 2)
- **Class:** A (OTAA)
- **Join Mode:** OTAA with configurable DevEUI / AppEUI / AppKey
- **Remote calibration:** via `AT+CALIBREMOTE` and downlink opcode `0xA0`
- **Low power:** STOP mode with RTC wakeup (<20 µA target)
- **Storage:** NVMM/EEPROM emulation with CRC32 validation
- **Dispatcher:** Table-driven downlink command system
- **Encoder:** Structured uplink encoder incl. OEM-style Power Profile (F3)
- **Application Offset:** **`0x0800F000`** (OEM-aligned memory layout)
- **Build System:** GNU Make + arm-none-eabi-gcc

---

# 🚀 Quick Start

## 1. Build the firmware

```bash
make clean && make -j4
```

Artifacts appear in `build/`, notably:

- `build/ais01.bin`
- `build/ais01.hex`
- `build/ais01.elf`

---

## 2. Flash the device

The firmware aligns with the OEM bootloader memory map:

- **Bootloader:** `0x08000000 – 0x0800EFFF`
- **Application:** `0x0800F000 – end`
- **Vector Table:** `0x0800F000`

### Flash with ST-Link:
```bash
st-flash write build/ais01.bin 0x0800F000
```

Or use the Dragino OTA Tool and select the built `.bin`.

---

## 3. Connect to serial console

```
minicom -D /dev/ttyUSB0 -b 115200
```

The device will print boot logs, version info, and AT responses.

---

## 4. Provision LoRaWAN credentials (OTAA)

```text
AT+DEVEUI=0102030405060708
AT+APPEUI=1122334455667788
AT+APPKEY=00112233445566778899AABBCCDDEEFF
AT+JOIN
```

Once joined, periodic uplinks begin automatically based on configured TDC.

---

# 📡 Core AT Commands

| Command | Description |
|---------|-------------|
| `AT` | Ping the parser |
| `AT+VER` | Firmware version & band |
| `AT+DEVEUI`, `AT+APPEUI`, `AT+APPKEY` | OTAA credentials |
| `AT+JOIN` | Start OTAA join |
| `AT+TDC=<ms>` | Uplink interval |
| `AT+ADR=<0/1>` | Enable ADR |
| `AT+PORT=<n>` | Set application port |
| `AT+CALIBREMOTE=<hex>` | Remote calibration (apply/query) |
| `ATZ` | Soft reboot |

Full AT surface lives in:

```
docs/firmware/specification/AT_Commands_Specification.md
```

---

# 🔧 Project Structure

```
AIS01-Lorawan-EndNode/
├── src/
│   ├── app/          # Application logic, AT interface, storage, calibration, scheduler
│   ├── board/        # STM32L072 + SX1276 hardware (GPIO, SPI, UART, RTC, power)
│   ├── cmsis/        # CMSIS + startup + system clock
│   ├── lorawan/      # Semtech LoRaMAC-node stack (region AU915, crypto, MAC)
│   ├── radio/        # SX1276 driver
│   └── system/       # Portable util modules (timers, fifo, nvmm, HAL stubs)
├── docs/
│   ├── firmware/     # Active documentation (specs + implementation notes)
│   └── legacy/       # OEM reverse engineering + historical analysis
├── Makefile          # Build system
└── stm32l072xx_flash_app.ld  # Linker (app offset = 0x0800F000)
```

---

# 🔍 Documentation (Active)

The **active firmware documentation** lives here:

```
docs/firmware/
```

Key files:

- **Memory_Map.md** — *Source of truth* for memory layout  
- **Firmware_Overview.md** — FSM + module overview  
- **specification/Architecture_Specification.md** — (to be rewritten, Phase 1)  
- **specification/AT_Commands_Specification.md** — AT surface (rewrite planned)  
- **implementation/Downlink_Dispatcher.md** — Handler table (rewrite planned)  
- **implementation/Calibration_Engine.md** — Calibration pipeline  
- **implementation/Hardware_Power.md** — STOP-mode + wakeup logic  
- **implementation/Scheduler.md** — Event-driven run/sleep loop  

OEM reverse-engineering, vector tables, strings, Ghidra artifacts, etc.  
are archived under:

```
docs/legacy/
```

These are **not** part of the firmware’s active design.

---

# 🔋 Power Consumption Targets

| Mode | Target Current | Notes |
|------|----------------|-------|
| TX | ~120 mA | Depends on TX power & region |
| RUN | ~1–2 mA | MCU active, peripherals enabled |
| STOP | **<20 µA** | RTC + SRAM retention only |

The STOP-mode pathway is implemented in:

```
src/app/power.c
docs/firmware/implementation/Hardware_Power.md
```

---

# 📦 Versioning

Current development version:  
```
1.0.0-dev
```

---

# 🙌 Credits

Developed by **Waterplan**  
LoRaMAC-node © Semtech  

---