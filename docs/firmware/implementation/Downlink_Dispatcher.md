# Downlink Dispatcher  
## AIS01-LB Custom Firmware

This module interprets downlink payloads received via LoRaWAN  
and routes them to the correct handler using a table-driven dispatcher.

---

# 1. Architecture

```
fport → opcode → validator → handler
```

- Each opcode has:
  - minimum length  
  - validation function  
  - handler function  
  - optional NVMM write  
  - optional uplink ACK  

Dispatcher file:  
`src/app/downlink_dispatcher.c`

---

# 2. Opcode Table (Authoritative)

| Opcode | Description | Handler | Notes |
|--------|-------------|----------|-------|
| `0xA0` | Remote Calibration | `HandleCalibration()` | persists NVMM + ACK uplink |
| `0x01` | Request Uplink | `HandleForceUplink()` | schedules immediate TX |
| `0x21` | Set DR/TXP | `HandleRadioConfig()` | optional depending on build |
| `0x31` | Set TDC (TX interval) | `HandleSetTDC()` | writes to NVMM |
| `0xF0` | Factory Reset | `HandleFactoryReset()` | resets config but keeps keys |

(*Opcodes not implemented here must NOT be accepted silently.*)

---

# 3. Flow

```
downlink_received()
→ decode fport
→ read opcode
→ lookup table
→ validate length
→ validator_fn(payload)
→ handler_fn(payload)
→ if needed: NVMM write
→ if needed: schedule uplink ACK
```

---

# 4. Calibration Opcode (0xA0)

- payload structure depends on calibration design  
- validated through Calibration Engine  
- persists snapshot in NVMM  
- schedules calibration ACK  
- returns success/failure status  

See: `Calibration_Engine.md`

---

# 5. Error Handling

Invalid opcode:
```
ignored + log
```

Invalid length or validator fail:
```
ignored + log
```

Downlinks produce NO AT errors — these errors are internal only.

---

# Source of Truth
This document matches the logic inside `downlink_dispatcher.c`.
