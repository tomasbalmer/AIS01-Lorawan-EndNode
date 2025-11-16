# Downlink Golden Test — Opcode 0x31 (Set TDC)

## Purpose
Validates downlink-driven uplink interval configuration.

---

# 1. Downlink Example
```
FPort: 1
Payload: 31 00 EA 60
```

Interpretation:
- `31` opcode
- `00 EA 60` → 60000 ms (0x00EA60)

---

# 2. Expected Behavior
1. Dispatcher routes opcode `0x31`
2. Extract 24-bit BE or LE (as implemented)
3. Validate TDC range (> 4000 ms)
4. Update RAM state
5. Persist into NVMM
6. Reset scheduler next-wake time

---

# 3. Invalid Payloads
```
31 00 00 01   (too small)
31 FF FF FF   (overflow)
```

Expected:
- discard packet
- NO NVMM writes
- NO scheduler changes

---
