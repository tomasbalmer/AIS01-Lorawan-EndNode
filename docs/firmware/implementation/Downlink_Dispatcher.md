# NOTE:
# This file will be rewritten to reflect current dispatcher code.

# Downlink Dispatcher (Opcode Table)

## Table Summary
- **Base address:** `0x08014ABC`
- **Entries:** 32-bit pointers (at least 16 slots observed; more likely extend into `0x08014B00–0x08014B80`).
- **Access pattern:** `opcode_parse_uint` (`0x080123E0`) loads payload pointer from `0x2000013C`, validates opcode range (`1..0x24`), masks invalid markers, then looks up handler pointer via table base (`DAT_080123DC = 0x0802199D`).

| Index | Value / Target | Notes |
|-------|----------------|-------|
| 0 | `0x0800D124+1` | `LAB_0800D124` handler stub |
| 1 | `0x0800D101` | data reference |
| 2 | `0x00000007` | constant used during validation |
| 3 | `0x20004B08` | pointer to RAM structure |
| 8 | `0xC8010001` | encoded flag field (bitmask) |
| 9 | `0x080121A8+1` | tail of `FUN_08012118` (payload copier) |
|10 | `opcode_handler_flag_and_timer+1` (`0x08011FE1+1`) | sets status flag & starts timer |
|11 | `opcode_validator_memcmp+1` (`0x080121E0+1`) | compares payload vs stored copy |
|12 | `0x0801220F` | literal `':'` used in diagnostics |
|13 | string " Delay between the end of the Tx..." | error/help message |
|14 | `opcode_parse_uint+1` (`0x080123E0+1`) | dispatcher re-entry |
|15 | `0x080120B5` (ASCII `'W'`) | constant / status marker |
|... | ... | further entries TBD |

## Key Functions
- `opcode_handler_flag_and_timer` (`0x08011FE1`)
  - Sets `unaff_r4[3] = 0x02`.
  - Starts timer located at `DAT_080120B8` → RAM `0x20006834`.
- `opcode_validator_memcmp` (`0x080121E0`)
  - Compares payload bytes against stored copy at offset `0x70`.
  - Branches to `LAB_08012226` when mismatch.
- `opcode_parse_uint` (`0x080123E0`)
  - Entry point for downlink processing.
  - Reads payload pointer from `0x2000013C` (`DAT_080123F4`).
  - Validates opcode range; loads handler pointer from `0x0802199D`.
  - Calls through handler after optional validation.
- `LAB_080122B4`
  - Core dispatch loop (sets up masking, issues indirect branch).
- `FUN_08012118`
  - Payload copy helper referenced by entry `9`.

## Related Globals
- `DAT_080120B8` → `0x20006834` (timer handle / context).
- `DAT_080120C0` → `0x200058D4` (parameter for `FUN_0800D768`).
- `DAT_080123DC` → `0x0802199D` (handler table pointer).
- `DAT_080123F4` → `0x2000013C` (current payload buffer pointer).

## Dispatcher Flow (Pseudo)
```c
uint8_t *payload = *(uint8_t **)0x2000013C;
uint8_t opcode = payload[0];
if (opcode == 0 || opcode > 0x24) {
    return;  // invalid
}
Handler handler = handlerTable[opcode];
if (opcode_validator_memcmp(payload, ... ) == 0) {
    handler(payload);
}
```

## Pending Analysis
- Complete table dump beyond the first 16 entries to map all opcodes.
- Resolve external handler at `0x080205EA` (table index unknown; outside binary range).
- Cross-reference opcode values with documented downlink commands (0x01 TDC, 0x21 ADR, 0xA0 calibration, etc.).
- Link handler functions to AT command counterparts to ensure mirrored validation/persistence.
