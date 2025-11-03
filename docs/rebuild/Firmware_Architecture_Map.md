# Firmware Architecture Map — Dragino AIS01-LB

| Module | Address Range | Key Entry Points | Linked Doc |
|--------|---------------|------------------|------------|
| Boot & Main State Machine | `0x08004000–0x080051FF` | Reset vector (`0x0800F30D`), state dispatcher (Loop around `FUN_0000F28C`) | Existing: `docs/ARCHITECTURE.md` |
| LoRaWAN Core | `0x08005214–0x08005B68` | `FUN_00001214` (StackInit), `FUN_0000122C` (Join), `FUN_00001238` (Uplink), `FUN_00001244` (Downlink) | `docs/rebuild/Lorawan_Core.md` |
| AT Command Layer | `0x08006000–0x080064A4` | `FUN_00002000` (Parser), `FUN_00002030` (`AT+JOIN`), `FUN_0000203C` (`AT+TDC`), … `FUN_00002474` (Region helpers) | `docs/rebuild/AT_Handlers.md` |
| Hardware & Power | `0x08007000–0x0800742C` | `FUN_00003000` (Init), `FUN_00003018` (STOP mode), `FUN_00003030` (Storage), `FUN_00003060` (Calibration) | `docs/rebuild/Hardware_Power.md` |
| Data Tables & Strings | `0x08014500–0x080183FF` | AT command names/help (`0x08016A06`), user messages (`0x08017300+`) | `docs/AIS01_bin_analysis/AIS01_strings.csv` |
| EEPROM Shadow | `0x08080800–0x08080FFF` | Config block consumed by storage routines | `docs/AIS01_bin_analysis/AIS01_nvm_map.txt` |

## Execution Flow Overview
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

## Cross-Reference Index
- **Function catalogue:** `docs/AIS01_bin_analysis/AIS01_function_analysis.md`
- **String lookup:** `docs/AIS01_bin_analysis/AIS01_strings.csv`
- **Pointer tables:** `docs/AIS01_bin_analysis/AIS01_pointers.csv` (for AT table @`0x08016A06`)
- **Interrupt vectors:** `docs/AIS01_bin_analysis/AIS01_vectors.csv` (links RTC/LPTIM IRQs to LoRa scheduler)
- **Reverse-engineering notes:** `docs/AIS01_bin_analysis/AIS01_overview.md`, `AIS01_extraction_plan.md`

## Usage Notes for Re-Implementation
1. **Preserve address-driven behaviour:** Many helpers (e.g., `write key error` handlers at `0x08006480`) correspond to validation flows that we should imitate functionally even if addresses change in the custom firmware.
2. **Reproduce textual outputs:** Strings in `AIS01_strings.csv` map 1:1 to handler outcomes. Keeping these messages intact ensures compatibility with Dragino tools and automated scripts.
3. **Align AT and downlink paths:** The OEM firmware intentionally funnels both paths through shared validators. In the rewrite, mirror this structure to avoid state divergence between field configuration (UART) and remote management (downlinks).
4. **Power discipline:** The STOP-mode path hinges on a clean handshake between the scheduler (`0x0800525C`) and hardware manager (`0x08007018`). Any custom logic should respect this sequence to hit the sub-20 µA target.
5. **Calibration workflow:** Remote calibration sits at the intersection of downlinks (`opcode 0xA0`) and AT commands. The calibration engine marks “pending apply” state until next reboot or explicit `ATZ`; replicate this to match the original behaviour hinted at `docs/AIS01_bin_analysis/AIS01_overview.md`.

