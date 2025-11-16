# Downlink Golden Test — Opcode 0xF0 (Factory Reset)

## Purpose
Validates handling of factory-reset command.

---

# 1. Example
```
Payload: F0
```

---

# 2. Expected Behavior
1. Reset runtime config (TDC, DR, TXP, ADR)
2. Preserve session keys and OTAA credentials
3. Write updated block to NVMM
4. Rejoin is NOT forced automatically
5. Next uplink MUST reflect new defaults

---

# 3. Edge Case
Factory reset during JOIN → apply but keep join state.
