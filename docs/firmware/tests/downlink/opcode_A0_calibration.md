# Downlink Golden Test — Opcode 0xA0 (Calibration)

## Purpose
Validates the full calibration pipeline:
- payload decoding
- validation
- application to RAM
- NVMM persistence
- ACK uplink scheduling

---

# 1. Downlink Example
```
FPort: 1
Payload: A0 01 12 34 56 78
```

Where:
- `A0`     opcode
- `01`     calibration version
- `12345678` example calibration block

---

# 2. Expected Behavior

## Step-by-step
1. Dispatcher routes opcode `0xA0`  
2. Validation function checks:
   - length ≥ 6
   - version valid
   - payload structure consistent
3. Calibration Engine:
   - writes new values into RAM
   - recomputes CRC32 for NVMM block
4. Storage writes to flash
5. Schedules Calibration ACK uplink

---

# 3. Expected NVMM Result

NVMM block must contain:
- header (internal)
- version
- calibration bytes
- CRC32 (recomputed)

Example CRC (dummy):
```
0x5A7C21EE
```

---

# 4. Expected ACK Uplink

Payload:
```
A0 00 <crc_hi> <crc_lo>
```

Status:
- `00` = OK
- `01` = Validation error

---

# 5. Rejection Cases

### Invalid payload (too short)
```
A0 01
```
Expected:
- payload discarded
- no NVMM change
- no ACK scheduled

### Bad structure
```
A0 FF FF FF
```
Expected:
- discard
- no flash writes

---
