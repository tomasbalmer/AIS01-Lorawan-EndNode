# Simulation — Scheduler Timeline

## Initial Conditions
```
joined = true
TDC = 60000 ms
force_uplink = false
ack_pending = false
```

## Expected Timeline
```
t=0s   → STOP
t=60s  → WAKE
t=60s  → TX (F3 + status)
t=61s  → STOP
t=121s → WAKE
t=121s → TX
...
```

## Pass Criteria
- STOP must precede TX
- TX must NOT repeat twice without STOP
- No drift > ±2 seconds per cycle

---
