# Downlink Golden Test — Opcode 0x01 (Force Uplink)

## Purpose
Triggers immediate uplink from device.

---

# 1. Downlink Example
```
FPort: 1
Payload: 01
```

---

# 2. Expected Behavior
1. Dispatcher identifies opcode = 0x01  
2. Schedules immediate uplink  
3. Device should TX next possible opportunity  
4. NO NVMM writes  
5. NO errors generated   

---

# 3. Edge Cases
### During JOIN
- Must wait until join completes
- Uplink should fire immediately after join

### During existing TX
- Queue "next uplink ASAP"
