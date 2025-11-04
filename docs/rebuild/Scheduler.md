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
```
bool scheduler_loop(void)
{
    if (g_ctx->boot_state == -1)
    {
        return false;                 // nothing scheduled yet
    }

    if (compute_transmit_offset_or_join_window() == false)
    {
        return false;                 // honour duty/join gate
    }

    if (g_ctx->next_op == OP_TX)
    {
        prepare_frame();              // builds MAC frame buffers
        lorawan_send();                // hands off to radio layer
        g_ctx->next_op = OP_IDLE;
    }
    else if (g_ctx->next_op == OP_RX)
    {
        update_rx_state();            // handled asynchronously
        g_ctx->next_op = OP_IDLE;
    }

    uint32_t delay = compute_next_idle_delay(g_ctx->scheduler_slot);
    if (delay > 0)
    {
        timer_set_next_wakeup(delay);
        timer_start();
    }

    return true;
}
```

## Notes
- `g_ctx->boot_state` (offset `0x12`) is initialised to `-1` until the first
  scheduler task is registered.
- `g_ctx->next_op` lives near offset `0x28` and is fed by both AT handlers and
  downlink opcodes that queue transmissions.
- `g_ctx->scheduler_slot` references timer descriptors stored around
  `0x200051E0`; actual hardware timers are configured by `FUN_0800F5BC`.
- External thunk `FUN_0800F454` (jumping to `0x0801FDBC`) remains unresolved
  in this dump; the rewritten firmware should provide an equivalent callback or
  stub for future integration.



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
