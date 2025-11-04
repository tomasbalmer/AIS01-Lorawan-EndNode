# Firmware Architecture Map — Dragino AIS01-LB

| Module | Address Range | Key Entry Points | Linked Doc |
|--------|---------------|------------------|------------|
| Boot & Main State Machine | `0x08004000–0x080051FF` | Reset vector (`0x0800F30D`), state dispatcher (Loop around `FUN_0000F28C`) | Existing: `docs/ARCHITECTURE.md` |
| LoRaWAN Core | `0x08005214–0x08005B68` | `FUN_00001214` (StackInit), `FUN_0000122C` (Join), `FUN_00001238` (Uplink), `FUN_00001244` (Downlink) | `docs/rebuild/Lorawan_Core.md` |
| AT Command Layer | `0x08006000–0x080064A4` | `FUN_00002000` (Parser), `FUN_00002030` (`AT+JOIN`), `FUN_0000203C` (`AT+TDC`), … `FUN_00002474` (Region helpers) | `docs/rebuild/AT_Handlers.md` |
| Hardware & Power | `0x08007000–0x0800742C` | `FUN_00003000` (Init), `FUN_00003018` (STOP mode), `FUN_00003030` (Storage), `FUN_00003060` (Calibration) | `docs/rebuild/Hardware_Power.md` |
| Data Tables & Strings | `0x08014500–0x080183FF` | AT command names/help (`0x08016A06`), user messages (`0x08017300+`) | `docs/AIS01_bin_analysis/AIS01_strings.csv` |
| Downlink Opcode Table | `0x08014ABC` (+) | Handler pointers for opcodes (`0x01…0x24`); dispatcher @ `0x080123E0` | `docs/rebuild/Downlink_Dispatcher.md` |
| EEPROM Shadow | `0x08080800–0x08080FFF` | Config block consumed by storage routines | `docs/AIS01_bin_analysis/AIS01_nvm_map.txt` |

**Binary coverage reminder:** current dump spans `0x08000000–0x0801502F`. Any reference to `0x0801xxxx` (for example `0x0801FDBC`) belongs to external code not present in this image; document such jumps as _external dependency_.

## Boot Initialisation Insights
- `Reset_Handler` at `0x0800F30C` sets boot flags in SRAM (`0x20000118`), mirrors `device_state_t` from `0x20006CC0` to `0x20006D0C`, clears the `ram_config_t` block at `0x20005140`, and zeroes scheduler list nodes via `*(node + 0x24) = 0`.
- Scheduler setup then flows through `FUN_0800F5BC` (timer guard) and `FUN_0800F4F4` (runtime snapshot) before touching `FUN_0800F454`, which is a thunk into code outside the dump.
- Additional helpers nearby:
  - `check_duty_or_join_gate` (`0x0800F308`) resets duty/join counters before transmissions.
  - `compute_transmit_offset_or_join_window` (`0x0800F380`) returns `0/1` to gate TX/join attempts.
  - `scheduler_loop` (`0x0800FD2C`) is the main scheduler; it exits early on init, queries join gate, orchestrates TX/RX preparation, and schedules STOP-mode delays via `compute_next_idle_delay` (`~0x0800FCA*`).

## RAM Structures (from Ghidra snapshot)
- **`ram_config_t` @ `0x20005140`:** status byte at `0x00`, list head pointer at `0x1C`, scheduler state flags at `0x28/0x2D`, wake timers at `0xA8/0xAC`, plus helper bytes (`0x63`, `0xB0`) referenced by timer routines.
- **`device_state_t` @ `0x20006CC0`:** boot flags at base, fields `0x24` and `0x2D` synced into `ram_config`, embedded sub-struct at `0x4C`, runtime counters at `0x70/0x74/0x78`, and mirrored copy at `0x20006D0C`.

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
            ↳ Preceded at boot by Reset_Handler → FUN_0800F5BC → FUN_0800F4F4 → FUN_0800F454 (external)
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
4. **Power discipline:** The STOP-mode path hinges on a clean handshake between the scheduler (`0x0800525C`) and hardware manager (`0x08007018`). Any custom logic should respect this sequence to hit the sub-20 µA target.
5. **Calibration workflow:** Remote calibration sits at the intersection of downlinks (`opcode 0xA0`) and AT commands. The calibration engine marks “pending apply” state until next reboot or explicit `ATZ`; replicate this to match the original behaviour hinted at `docs/AIS01_bin_analysis/AIS01_overview.md`.
6. **External dependencies outstanding:** Calls resolving through `FUN_0800F454` and opcode-table jumps to `0x080205EA` require the additional image covering `0x0801xxxx`; plan acquisition or stubbing before final integration.
