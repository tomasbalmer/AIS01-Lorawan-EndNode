# Scheduler  
## AIS01-LB Custom Firmware

The scheduler determines when uplinks occur and when to enter STOP mode.

---

# 1. Responsibility

- compute next TX deadline  
- schedule STOP mode  
- wake via RTC  
- enforce OTAA join before TX  
- handle forced uplinks  
- handle calibration ACK uplinks  

---

# 2. Main Loop

```
while (1):
    if not joined:
        attempt_join()
        continue

    next = compute_next_tx_time()
    enter_stop_until(next)

    wake()
    build_uplink()
    tx()

    process_downlink()
```

---

# 3. STOP Mode Cycle

STOP enters:
- RTC ON  
- SRAM retained  
- peripheral clocks OFF  
- SX1276 OFF  

Wake events:
- RTC alarm  
- external interrupt (if enabled)  

---

# 4. Timing Rules

- TDC defines periodic uplink intervals  
- calibration ACK forces next uplink  
- join procedure preempts scheduler  

---

# Source of Truth
Matches `scheduler.c`, `power.c`, and RTC wake logic.
