# NOTE:
# To be aligned with new calibration pipeline.

# Calibration / Configuration Apply Engine

## Overview
- Handles LoRaWAN downlink payloads that adjust internal configuration (“calibration”).
- Payloads arrive via opcode dispatcher and are buffered at `0x20004B08` (mirror `0x20004D98`).
- Applies values to peripherals in the `0x4001F000` / `0x4001FC00` region and acknowledges via uplink.

## Key Functions
| Function | Address | Description |
|----------|---------|-------------|
| `FUN_08003030` | `0x08003030` | Selects target peripheral base and primes context. |
| `FUN_08003060` | `0x08003060` | Updates 4-bit fields at `[ctx+0x38/0x3C/0x40]`; manages `pending_apply` flags. |
| `FUN_080030D4` | `0x080030D4` | Busy-wait until `0x4001D000 & 0xF == 0xF` (hardware ready). |
| `FUN_080030E8` | `0x080030E8` | Commits configuration: sets `[ctx+0x18]=1`, toggles bit `0x20` at `0x40000024`. |
| `FUN_0800D100` | `0x0800D100` | Opcode handler that parses payload and invokes apply/query paths. |
| `opcode_validator_memcmp` | `0x080121E0` | Validates mirror vs. active configuration (word-by-word compare). |

## Payload Buffer Layout (`0x20004B08`)
| Offset | Size | Field | Notes |
|--------|------|-------|-------|
| `0x00` | 1 byte | `cmd` | `0` = query/verify, `1` = apply |
| `0x01` | 1 byte | `flags` | Operation subtype |
| `0x04` | 4 bytes | `param` | Channel/index parameter |
| `0x10` | 4 bytes | `value` | Configuration/calibration value |
| `0x18` | 4 bytes | `apply_flag` | Set to `1` during commit |

## Execution Flow
1. Downlink dispatcher copies payload to `0x20004B08` and updates mirror at `0x20004D98`.
2. `cmd == 0`: verification path uses `FUN_08003030` → `FUN_08003060`.
3. `cmd == 1`: apply path calls `FUN_080030E8` after setting pending flags.
4. `FUN_080030D4` waits for hardware ready; bit `0x20` at `0x40000024` enables the configuration block.
5. `FUN_0800BC5C(0x8B)` sends uplink / AT response (e.g., `CFG_OK`, `CAL_OK`).

## Flags & States
- `pending_apply`: stored in nibbles at `[ctx+0x38]`, `[ctx+0x3C]`, `[ctx+0x40]`; values manipulated via bitmasks (no ms conversion).
- `apply_busy`: `[ctx+0x18] = 1` while hardware commit in progress.
- Hardware enable: bit `0x20` at `0x40000024` toggled during apply.

## Integrity & Persistence
- Integrity relies on `opcode_validator_memcmp` comparing mirror and active values (no CRC polynomial).
- No Flash/EEPROM writes observed during calibration apply; configuration exists only in RAM + hardware unless the rewritten firmware adds persistence.

## Memory / Peripheral References
- `0x4001F000`, `0x4001FC00`: register blocks updated during apply.
- `0x4001D000`: ready/status register (low nibble).
- `0x40000024`: control register with bit `0x20`.
- `0x20004B08`: payload buffer (working copy).
- `0x20004D98`: mirror pointer/component index.

Implementation hook: `src/board/calibration_hw.c` provides the `Calibration_HwWaitReady` /
`Calibration_HwSetEnable` functions that manipulate these registers with a simple timeout
and bit toggle sequence.

## Outstanding Items
- Opcode table entry pointing to external handler `0x080205EA` (acknowledgement helper residing outside current dump).
- Confirm text strings used for acknowledgments (`CFG_OK`, `CAL_OK`) for AT parity.
