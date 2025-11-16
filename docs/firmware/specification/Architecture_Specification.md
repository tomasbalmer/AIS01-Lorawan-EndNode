# Architecture Specification  
## AIS01-LB Custom Firmware (OEM-Compatible Layout)

This document defines the architecture of the AIS01-LB custom firmware.  
It is the **source of truth** for system behavior, modules, FSM flow, memory layout,  
and the relationships between core components.

All OEM reverse-engineering notes live under `docs/legacy/`  
and **MUST NOT** be treated as authoritative.

---

# 1. Memory Map (Source of Truth)

The firmware follows the OEM memory layout:

- **Bootloader:** `0x08000000 – 0x0800EFFF`
- **Application:** `0x0800F000 – end of flash`
- **Vector Table:** `0x0800F000`

This is enforced by:

- `stm32l072xx_flash_app.ld`
- `src/system/system_stm32l0xx.c` (`VTOR = 0x0800F000`)

See `docs/firmware/Memory_Map.md` for full details.

---

# 2. Boot Flow

1. STM32L072 reset
2. OEM bootloader executes  
3. Bootloader jumps to `0x0800F000`
4. Custom firmware startup sequence:

```
SystemInit()
→ HAL init
→ Board_Init()
→ Storage_Init()
→ App_Init()
→ Enter FSM (BOOT → INIT)
```

---

# 3. Firmware Finite State Machine (FSM)

```
BOOT
→ INIT
→ JOIN
→ IDLE
→ UPLINK
→ SLEEP (STOP mode)
→ WAKE
→ IDLE
```

### BOOT  
Hardware init, storage loading, system sanity checks.

### INIT  
Prepares LoRaMAC-node, scheduler, timers, and AT services.

### JOIN  
OTAA join procedure (with retries and backoff).

### IDLE  
Main steady state.  
Schedules uplinks, reacts to AT commands, and enters STOP.

### UPLINK  
Builds and transmits:
- periodic telemetries
- calibration ACKs
- OEM-style Power Profile (F3)
- status reports

### SLEEP  
MCU enters STOP mode with:
- RTC running
- SRAM retention
- SX1276 powered off

Targets **<20 µA**.

### WAKE  
RTC wakes the device, scheduler resumes, next IDLE.

---

# 4. Core Modules

## 4.1 AT Interface  
Location: `src/app/atcmd.c`

- Token-based parser
- Maps AT commands → handlers
- Password-protected mode (optional)
- Error model:
  - `AT_ERROR`
  - `AT_PARAM_ERROR`
  - `AT_BUSY_ERROR`
  - `AT_NO_NET_JOINED`

Full AT list: `docs/firmware/specification/AT_Commands_Specification.md`

---

## 4.2 Storage / NVMM  
Location: `src/app/storage.c`

- EEPROM emulation using flash pages
- CRC32 validation
- Stores:
  - DevEUI / AppEUI / AppKey
  - LoRaWAN session keys
  - DR / TXP / ADR settings
  - calibration data snapshot
  - frame counters

Boot process:
```
Load → Validate CRC → Apply → If invalid → reset to defaults
```

---

## 4.3 Calibration Engine  
Location: `src/app/calibration.c`

- Handles remote calibration payload
- Applies changes into RAM + persistent NVMM
- Downlink opcode: **0xA0**
- Uplink ACK frame emitted after success

Details in:
`docs/firmware/implementation/Calibration_Engine.md`

---

## 4.4 Downlink Dispatcher  
Location: `src/app/downlink_dispatcher.c`

- Table-driven command handler:
  ```
  opcode, min_len, validator, handler_fn
  ```
- Handles:
  - calibration
  - uplink request
  - config changes
- Provides MAC results into the app layer

Details in:
`docs/firmware/implementation/Downlink_Dispatcher.md`

---

## 4.5 Uplink Encoder  
Location: `src/app/uplink_encoder.c`

Builds structured payloads including:

- **Power Profile (0xF3)** OEM-style frame:
  ```
  Battery %, Battery mV
  MCU uptime
  Sensor PWR flag
  LoRa DR
  ```
- Status frame
- Sensor readings (if available)
- Calibration ACK

---

## 4.6 Scheduler  
Location: `src/app/scheduler.c`

- Controls periodic uplinks  
- Defines next wake event  
- Interacts with STOP mode

---

## 4.7 Power Management  
Location: `src/app/power.c`

- STOP mode entry/exit
- SX1276 power gating
- Regulator-enabled pathways
- Watchdog refresh (IWDG ~32s)

Targets:
- RUN: ~1–2 mA  
- STOP: **<20 µA**

---

## 4.8 LoRaWAN Stack  
Location: `src/lorawan/`

Based on Semtech LoRaMAC-node DR-LWS-007:

- MAC layer  
- Crypto  
- Region AU915-SubBand2  
- RX1/RX2 timing  
- ADR  

---

# 5. Event Flow Summary

1. Device boots  
2. Loads NVMM  
3. JOIN or RESTORE session  
4. Scheduler computes next TX deadline  
5. Device enters STOP  
6. RTC wakeup  
7. Build + send UPLINK  
8. Process downlinks  
9. Loop to IDLE

---

# 6. OTA Compatibility

This firmware:
- uses OEM memory offset  
- preserves bootloader region  
- does **NOT** implement OEM OTA  
- future OTA is possible but out of scope  

---

# 7. Testing

Recommended:

- AT surface test script  
- Golden uplink frames  
- Downlink injection tests  
- STOP-mode current measurement  
- Join flow validation  

---

# 8. Source of Truth

This file + the codebase are authoritative.  
Everything in `docs/legacy/` is historical and must NOT influence firmware behavior.
