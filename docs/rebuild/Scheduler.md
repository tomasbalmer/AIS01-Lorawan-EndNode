# Scheduler & Duty Cycle Notes

## Overview
- **Context base pointer:** `g_ctx` at `0x20005140`
- **Main loop:** `scheduler_loop` at `0x0800FD2C`
- **Related helpers:**
  - `Reset_Handler` @ `0x0800F30C`
  - `check_duty_or_join_gate` @ `0x0800F308`
  - `compute_transmit_offset_or_join_window` @ `0x0800F380`
  - `FUN_0800F5BC`/`FUN_0800F4F4` (timer and snapshot)
  - `compute_next_idle_delay` ~ `0x0800FCA*`
  - External thunk via `FUN_0800F454` -> `0x0801FDBC`

## Reset & Context Initialisation
- `Reset_Handler` (`0x0800F30C`):
  - Sets boot flags at `0x20000118`
  - Mirrors `device_state_t` (`0x20006CC0`) into runtime copy (`0x20006D0C`)
  - Clears `ram_config_t` (`0x20005140`)
  - Walks scheduler list: `*(node + 0x24) = 0; node = *(node + 0x28)`
  - No state transitions; only RAM prep

## Duty/Join Helpers
- `check_duty_or_join_gate` (`0x0800F308`)
  - Resets duty-cycle/join counters in RAM
  - Repeats init pattern without touching radio modules
- `compute_transmit_offset_or_join_window` (`0x0800F380`)
  - Reads offsets from `DAT_0800F3D0` (+0x22, +0x23)
  - Returns `0` (block) / `1` (allow)
  - Used as gate before TX/join attempts

## Scheduler Loop (`0x0800FD2C`)
- Steps (current understanding):
  1. If `g_ctx[0x12] == -1`, exit early (return `9`)
  2. Check join/duty gate via `compute_transmit_offset_or_join_window`
     - If blocked, return `8`
  3. Manage TX/RX: call payload builders and MAC send routines
  4. Idle path:
     - `compute_next_idle_delay(g_ctx[0x20])`
     - `timer_set_next_wakeup` + `timer_start`
  5. Low power integration with `Power` module (STOP mode)

## Outstanding Questions / TODO
- Precise pseudocode & variable naming for `scheduler_loop`
- Detailed layout of `g_ctx` struct (beyond known offsets)
- Behaviour of external jump via `FUN_0800F454`
- Interactions with LoRaMAC (MAC command scheduling, retry logic)
- Confirmation of timer units (ms vs ticks) in `0xA8/0xAC`

## Next Data Needed
- Full disassembly notes or pseudocode for `scheduler_loop`
- Mapping of opcodes at `0x08014A00` and their impact on scheduler state
- Calibration module effects on scheduler timers
- Storage persistence triggers related to duty-cycle counters



## to processs before adding

#### 0xXX — opcode_handler_flag_and_timer

**Address:** `0x08011FE1`  
**References:** Table[10] at 0x08014AE4  
**Calls 0x080205EA:** No  

**Behavior:**
- Writes constant `0x02` at offset 3 of payload buffer.
- If `payload[0] == 0` and dereferenced pointer at `(r5 + 8)` is 0:
  - Calls `FUN_0800D768(DAT_080120C0)` (likely memory sync/flush)
  - Starts timer at `DAT_080120B8`.

**Likely purpose:**  
Initialize or re-arm internal timer when a flag-based downlink command is received.

#### 0xXX — opcode_validator_memcmp

**Address:** `0x080121E1`  
**References:** Table[11] at `0x08014AE8`  
**Calls 0x080205EA:** No (directly)  

**Behavior:**
- Custom memory comparison between two data buffers.
- Operates like `memcmp(a, b, len)` but uses indirect pointers (`unaff_r4`, `unaff_r5`, etc.)
  linked to internal context (likely the StorageData or calibration payload).
- Returns:
  - `0` if equal,
  - or the byte difference otherwise.

**Likely purpose:**  
Used to validate CRC blocks or compare stored configuration (EEPROM or flash)  
against the incoming payload during a downlink command or calibration request.