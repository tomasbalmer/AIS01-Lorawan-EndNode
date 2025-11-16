# Firmware Architecture Specification — Dragino AIS01-LB

**Device:** Dragino AIS01-LB LoRaWAN AI Image End Node
**MCU:** STM32L072CZ
**Radio:** SX1276 (LoRa)
**Firmware Version:** v1.0.5
**LoRaWAN Stack:** DR-LWS-007 (AU915)

---

## Overview

The firmware is designed as a modular state machine for ultra-low-power LoRaWAN Class A operation with image capture and AI processing capabilities.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                        │
│  ┌────────────┐  ┌──────────┐  ┌────────────┐              │
│  │   main.c   │──│ atcmd.c  │──│ storage.c  │              │
│  │ (State     │  │ (AT CMD  │  │ (EEPROM    │              │
│  │  Machine)  │  │  Parser) │  │  Emul)     │              │
│  └─────┬──────┘  └────┬─────┘  └─────┬──────┘              │
│        │              │               │                      │
│  ┌─────▼──────────────▼───────────────▼──────┐              │
│  │         lorawan_app.c                     │              │
│  │  (LoRaWAN Application Layer)              │              │
│  └───────────────────┬───────────────────────┘              │
└────────────────────────┼─────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│          LoRaWAN MINIMAL STACK (src/lorawan)                 │
│  ┌──────────┐  ┌───────────┐  ┌─────────┐                  │
│  │ Core     │──│ Region    │──│ Crypto  │                  │
│  │ lorawan.c│  │ AU915     │  │ AES/CMAC│                  │
│  └─────┬────┘  └───────────┘  └─────────┘                  │
└────────┼──────────────────────────────────────────────────────┘
         │
┌────────▼──────────────────────────────────────────────────────┐
│              HAL / BOARD SUPPORT                              │
│  ┌─────────┐  ┌──────────┐  ┌────────┐  ┌────────┐          │
│  │ Radio   │  │ UART     │  │ RTC    │  │ Power  │          │
│  │ SX1276  │  │ HAL      │  │ Timer  │  │ Mgmt   │          │
│  └─────────┘  └──────────┘  └────────┘  └────────┘          │
└───────────────────────────────────────────────────────────────┘
```

---

## Memory Map

### Memory Map (Updated — OEM Verified via Reverse Engineering)

The actual OEM firmware (AU915) uses the following layout:

| Region       | Start       | End         | Notes                              |
|--------------|-------------|-------------|------------------------------------|
| Bootloader   | 0x08000000  | 0x0800EFFF  | OEM OTA Bootloader v1.4            |
| Application  | 0x0800F000  | End of flash| Reset_Handler at 0x0800F30D        |
| Vector Table | 0x0800F000  | —           | Application interrupt vectors      |
| Data EEPROM  | 0x08080000  | 0x08080FFF  | Configuration + keys (unchanged)   |

This firmware adopts **the same application offset (`0x0800F000`)** for full consistency.

**Note:** Binary dump analyzed covers `0x08000000–0x0801502F`. References to `0x0801xxxx+` are external dependencies (bootloader or precompiled libraries).

### RAM Layout (20 KB)

| Address Range | Size | Purpose |
|---------------|------|---------|
| `0x20000000–0x20003FFF` | 16 KB | Variables + Stack |
| `0x20004000–0x20004FFF` | 4 KB | Heap |

**Key RAM Structures (from Ghidra analysis):**

1. **`ram_config_t` @ `0x20005140`:**
   - `0x00`: Status byte
   - `0x1C`: List head pointer
   - `0x28`, `0x2D`: Scheduler state flags
   - `0xA8`, `0xAC`: Wake timers
   - Helper bytes: `0x63`, `0xB0` (referenced by timer routines)

2. **`device_state_t` @ `0x20006CC0`:**
   - Base: Boot flags
   - `0x24`, `0x2D`: Fields synced to `ram_config`
   - `0x4C`: Embedded sub-struct
   - `0x70`, `0x74`, `0x78`: Runtime counters
   - `0x20006D0C`: Mirrored copy

---

## Binary Memory Regions

**From Ghidra Analysis (`ghydra.txt`):**

- **Application Offset:** Dump starts at `0x0000` but loads at `0x0800_4000`
- **Image Size:** 0x15030 bytes (≈86 KB)
- **Vector Table:** `0x0800_4000–0x0800_40FF`
  - Reset → `0x0800F30D`
  - IRQs → `0x080147xx–0x080148xx` (common handlers)

**Code Sections:**

| Range | Purpose |
|-------|---------|
| `0x08004100–0x08005FFF` | Math/HAL primitives |
| `0x08006000–0x08008FFF` | Peripheral controllers (SPI/UART/RTC) |
| `0x08009000–0x0800DFFF` | LoRa event queue and radio code |
| `0x0800F000–0x08011FFF` | LoRaMAC layer and join/uplink management |
| `0x08012000–0x08013FFF` | AT parser logic |
| `0x08014000–0x080148FF` | Text tables, AT state, post-join routines |
| `0x08014900–0x08015FFF` | Working buffers (JPEG, packet storage) |
| `0x08016A00–0x08016BFF` | Command/help table |

---

## Module Architecture Map

| Module | Address Range | Key Entry Points | Reference Doc |
|--------|---------------|------------------|---------------|
| **Boot & Main State Machine** | `0x0800F000–0x080101FF` | Reset vector (`0x0800F30D`), state dispatcher (`FUN_0000F28C`) | `implementation/` modules |
| **LoRaWAN Core** | `0x08005214–0x08005B68` | StackInit (`0x00001214`), Join (`0x0000122C`), Uplink (`0x00001238`), Downlink (`0x00001244`) | `specification/LoRaWAN_Core_Specification.md` |
| **AT Command Layer** | `0x08006000–0x080064A4` | Parser (`0x00002000`), Handlers (`0x08006030+`) | `specification/AT_Commands_Specification.md` |
| **Hardware & Power** | `0x08007000–0x0800742C` | Init (`0x00003000`), STOP mode (`0x00003018`), Storage (`0x00003030`), Calibration (`0x00003060`) | `implementation/Hardware_Power.md`, `implementation/Calibration_Engine.md` |
| **Data Tables & Strings** | `0x08014500–0x080183FF` | AT command names/help (`0x08016A06`), user messages (`0x08017300+`) | `analysis/AIS01_strings.md` |
| **Downlink Opcode Table** | `0x08014ABC+` | Handler pointers for opcodes (`0x01…0x24`), dispatcher @ `0x080123E0` | `implementation/Downlink_Dispatcher.md` |
| **EEPROM Shadow** | `0x08080800–0x08080FFF` | Config block consumed by storage routines | `analysis/AIS01_nvm_map.md` |

---

## Main State Machine (main.c)

### Application States

```c
typedef enum {
    APP_STATE_BOOT,      // Module initialization
    APP_STATE_JOIN,      // OTAA join process
    APP_STATE_IDLE,      // Event waiting
    APP_STATE_UPLINK,    // Data transmission
    APP_STATE_RX,        // Downlink reception
    APP_STATE_SLEEP      // Low-power mode
} AppState_t;
```

### State Flow

```
BOOT → JOIN → IDLE ⇄ UPLINK → RX → SLEEP → IDLE
                ↑_____________________________│
                        (RTC Wake-up)
```

### Boot Initialization Sequence

**Reset_Handler @ `0x0800F30C`:**

1. Set boot flags in SRAM (`0x20000118`)
2. Mirror `device_state_t` from `0x20006CC0` to `0x20006D0C`
3. Clear `ram_config_t` block at `0x20005140`
4. Zero scheduler list nodes via `*(node + 0x24) = 0`

**Scheduler setup flow:**
```
Reset_Handler → FUN_0800F5BC (timer guard)
              → FUN_0800F4F4 (runtime snapshot)
              → FUN_0800F454 (external dependency)
```

**Additional helpers:**
- `check_duty_or_join_gate` (`0x0800F308`): Resets duty/join counters
- `compute_transmit_offset_or_join_window` (`0x0800F380`): Returns `0/1` to gate TX/join
- `scheduler_loop` (`0x0800FD2C`): Main scheduler; orchestrates TX/RX/STOP delays

---

## Execution Flow

```
Reset @0x0800F30D → SystemInit (clock/setup)
    ↓
Boot State (verify EEPROM via 0x08007030)
    ↓
Hardware Bring-up (SX1276, sensor, UART) @0x08007000
    ↓
Enter Main Loop (state machine around 0x0800F28C)
    ├─ LoRaWAN Join (0x0800522C) ← triggered by boot or AT+JOIN (0x08006030)
    ├─ Periodic Uplink (0x08005238) ← sensor read (0x0800703C) & timing from RTC (0x08007024)
    ├─ Downlink Handling (0x08005244) ← dispatch to config/power/calibration routines
    └─ Sleep Cycle (0x08007018) ← scheduled by window scheduler (0x0800525C)
```

---

## Key Modules

### 1. lorawan_app.c - LoRaWAN Application Layer

**Responsibility:** Bridge between application and LoRaWAN stack

**Key Functions:**
- `LoRaWANApp_Init()`: Load credentials from flash, prepare `lorawan` context
- `LoRaWANApp_Join()`: Trigger OTAA join
- `LoRaWANApp_SendUplink()`: Serialize and send payloads
- `LoRaWANApp_Process()`: Execute deferred stack tasks

**Callbacks:**
- `OnJoinSuccess() / OnJoinFailure()`: Join result
- `OnTxComplete()`: Transmission status
- `OnRxData()`: Downlink demultiplexing and opcode dispatch

### 2. atcmd.c - AT Command Parser

**Responsibility:** UART interface for configuration

**Structure:**
```c
typedef struct {
    const char *name;
    ATCmdHandler_t handler;
    const char *help;
} ATCmdEntry_t;

static const ATCmdEntry_t g_ATCmdTable[] = {
    { "AT+DEVEUI", ATCmd_HandleDevEUI, "..." },
    { "AT+JOIN", ATCmd_HandleJoin, "..." },
    // ... 67 total commands
};
```

**Processing Flow:**
```
UART RX → ProcessChar() → Buffer → Parse() → Lookup Table → Handler()
                                                               ↓
                                                           Response()
```

**Binary Implementation:**
- Parser entry: `FUN_00002000` @ `0x08006000`
- Command table: `0x08016A06–0x08016A68` (67 entries)
- Password gate: `"Enter Password to Active AT Commands"` @ `0x0801759A`

### 3. storage.c - Non-Volatile Storage

**Responsibility:** Configuration persistence in flash

**Data Structure:**
```c
typedef struct {
    uint32_t Magic;              // 0xDEADBEEF
    uint32_t Version;            // 1
    StorageData_t Data;          // Configuration
    uint32_t Crc;                // CRC32
} StorageBlock_t;
```

**Memory:**
- Base: `0x08080000` (STM32L072 data EEPROM area, 4KB)
- Byte-by-byte write via direct `DATA_EEPROM` access
- Direct read from memory-mapped region
- Stores: keys, session counters, AT configuration

**Binary Implementation:**
- Storage module: `0x08007030`
- NVM candidates: `0x08080808`, `0x08080809`, `0x0808090A` (see `analysis/AIS01_nvm_map.md`)

### 4. calibration.c - Remote Calibration

**Responsibility:** Remote sensor parameter adjustment

**Commands:**
```c
CALIB_CMD_RESET        = 0x01  // Reset to defaults
CALIB_CMD_SET_OFFSET   = 0x02  // Adjust offset
CALIB_CMD_SET_GAIN     = 0x03  // Adjust gain
CALIB_CMD_SET_THRESHOLD= 0x04  // Adjust threshold
CALIB_CMD_QUERY        = 0x05  // Query values
```

**Input Channels:**
1. AT command: `AT+CALIBREMOTE=<payload>`
2. LoRaWAN downlink: Port 10, Opcode `0xA0`

**Binary Implementation:**
- Opcode `0xA0` handler: `0x08007060`
- Apply message: `"Set after calibration time or take effect after ATZ"` @ `0x08017575`

### 5. power.c - Power Management

**Responsibility:** Ultra-low power consumption

**Power Modes:**
```c
POWER_MODE_RUN    // ~1.5 mA (CPU active)
POWER_MODE_SLEEP  // ~500 µA (WFI)
POWER_MODE_STOP   // <20 µA (RTC + flash standby)
```

**STOP Mode Flow:**
```
1. Disable peripherals (UART, GPIO clocks)
2. Put SX1276 in sleep
3. Configure RTC wake-up
4. Enter STOP mode (WFI)
5. [HARDWARE WAKEUP]
6. Restore system clock (HSI)
7. Re-enable peripherals
8. Continue execution
```

**Binary Implementation:**
- STOP mode entry: `0x08007018`
- Sleep command: `AT+SLEEP` @ `0x08006234`

---

## LoRaWAN Stack Integration

**Stack Components:**
- `lorawan.c`: Frame construction (MHDR, FHDR, MIC), counter management, RX windows
- `lorawan_crypto.c`: AES-128 and CMAC primitives (Join MIC, Frame MIC, payload encryption)
- `lorawan_region_au915.c`: Sub-band 2 channel tables, DR per channel, frequency hopping
- `lorawan_app.c`: Session persistence (DevAddr, keys, frame counters) and application callbacks

**Binary Addresses:**
- StackInit: `FUN_00001214` @ `0x08005214`
- Join handler: `FUN_0000122C` @ `0x0800522C`
- Uplink serializer: `FUN_00001238` @ `0x08005238`
- Downlink demux: `FUN_00001244` @ `0x08005244`
- ADR controller: `FUN_00001250` @ `0x08005250`
- RX window scheduler: `FUN_0000125C` @ `0x0800525C`
- AU915 channel calculator: `FUN_00001274` @ `0x08005274`

---

## Command & Configuration Topology

```
UART (AT) → Parser (0x08006000) → Handlers (0x0800600C…) →
    ├─ LoRa Core Updates (ADR/DR/TXP/TDC) @0x08005250/0x08005274
    ├─ Storage Commits @0x08007030 (keys, counters, calibration)
    ├─ Power Actions @0x08007018 (SLEEP) / 0x08007054 (BAT)
    └─ Sensor Ops @0x0800703C/48/60 (GETSENSORVALUE, CALIBREMOTE)

Downlink (opcodes 0x01,0x21,0xA0) → Dispatcher (0x08005244) →
    ├─ Shares validators with AT handlers (0x0800603C, 0x08006048…)
    ├─ Persists via storage (0x08007030)
    └─ Schedules acknowledgement uplink (0x08005238)
```

---

## Timing Constraints (LoRaWAN)

| Event | RX1 Delay | RX2 Delay | Notes |
|-------|-----------|-----------|-------|
| **Join Accept** | 5s | 6s | After Join Request |
| **Class A RX** | 1s | 2s | After Uplink |
| **Duty Cycle** | N/A | N/A | AU915 has no restriction |
| **ADR** | Dynamic | Dynamic | Automatic DR adaptation |

---

## Power Budget

| Event | Duration | Current | Energy |
|-------|----------|---------|--------|
| STOP mode | 59s | 15 µA | 0.885 mAh |
| Wake-up + Process | 100ms | 1.5 mA | 0.0042 mAh |
| LoRa TX (DR2) | 300ms | 120 mA | 10 mAh |
| RX1 Window | 100ms | 15 mA | 0.42 mAh |
| RX2 Window | 100ms | 15 mA | 0.42 mAh |
| **TOTAL (60s cycle)** | | | **11.7 mAh/cycle** |

**Battery Life (2400 mAh):**
- 60s cycle: ~14 days
- 300s cycle: ~60 days (optimized)

**Optimizations:**
- Increase TDC to 300s → +4x autonomy
- Disable confirmed messages → -20% consumption
- Use ADR for higher DR → shorter TX time

---

## Secure Boot Considerations

**Vector Table Relocation:**

Firmware starts at `0x0800F000` to preserve Dragino bootloader.

**Linker Script:**
```ld
MEMORY {
  FLASH (rx) : ORIGIN = 0x0800F000, LENGTH = 0x31000
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 20K
}
```

**System Init:**
```c
#define VECT_TAB_OFFSET  0xF000
SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
```

---

## Thread Safety

**Architecture:** Single-threaded, no RTOS

**Concurrency Model:**
- Main loop + interrupts
- Critical sections with `CRITICAL_SECTION_BEGIN/END`

**Interrupts (from `analysis/AIS01_vectors.csv`):**
```c
RTC_WKUP_IRQHandler()    // Wake from STOP
USART2_IRQHandler()      // UART RX
DIO0_IRQHandler()        // Radio TX done
DIO1_IRQHandler()        // Radio RX timeout
```

---

## Error Handling Strategy

**Fail-safe with automatic retry:**

| Error | Response |
|-------|----------|
| Join Failed | Retry with exponential backoff |
| TX Failed | Re-queue in next cycle |
| Storage CRC Fail | Load defaults, reinitialize |
| Radio Error | Reset radio, reinitialize |
| Watchdog Timeout | System reset |

---

## Debug Hooks

**Configuration:**
```c
// config.h
#define DEBUG_ENABLED  1        // Enable printf via UART
#define USE_RADIO_DEBUG        // Debug pins TX/RX toggle

// main.c
DEBUG_PRINT("Uplink sent: %d bytes\r\n", size);
```

**Production:**
- `DEBUG_ENABLED = 0` for smaller code size
- Logs only for critical errors

**Binary Debug:**
- `AT+DEBUG=1` @ `0x0800609C`: Enable verbose UART logs
- Message: `"Use AT+DEBUG to see more debug info"` @ `0x080175C1`

---

## Implementation Notes

### 1. Address-Driven Behavior
Preserve functional equivalents of binary handlers even if addresses change in custom firmware.

### 2. Textual Output Compatibility
Keep response strings identical to OEM firmware for compatibility with Dragino tools and automated scripts (see `analysis/AIS01_strings.md`).

### 3. AT and Downlink Path Alignment
Both UART and downlink paths share validators to avoid state divergence between local (UART) and remote (LoRaWAN) configuration.

### 4. Power Discipline
STOP-mode path requires clean handshake between scheduler (`0x0800525C`) and hardware manager (`0x08007018`) to achieve sub-20 µA target.

### 5. Calibration Workflow
Remote calibration marks "pending apply" state until next reboot or explicit `ATZ`.

### 6. External Dependencies
Calls resolving through `FUN_0800F454` and opcode-table jumps to `0x080205EA` require additional image covering `0x0801xxxx` range; plan acquisition or stubbing before final integration.

---

## Cross-Reference Index

- **Function catalogue:** `docs/analysis/AIS01_function_analysis.md`
- **String lookup:** `docs/analysis/AIS01_strings.md`
- **Pointer tables:** `docs/analysis/AIS01_pointers.csv`
- **Interrupt vectors:** `docs/analysis/AIS01_vectors.csv`
- **NVM map:** `docs/analysis/AIS01_nvm_map.md`
- **Reverse-engineering notes:** `docs/analysis/AIS01_overview.md`
- **AT Commands:** `docs/specification/AT_Commands_Specification.md`
- **LoRaWAN Core:** `docs/specification/LoRaWAN_Core_Specification.md`
- **Hardware:** `docs/specification/Hardware_Specification.md`
- **Implementation guides:** `docs/implementation/`

---

## Compatibility & Standards

- **LoRaWAN:** v1.0.3
- **Region:** AU915 (ACMA compliant)
- **Tested Networks:** TTN, ChirpStack
- **Bootloader:** Dragino proprietary (preserved at `0x08000000`)
- **OTA Tool:** Dragino Sensor Manager compatible
