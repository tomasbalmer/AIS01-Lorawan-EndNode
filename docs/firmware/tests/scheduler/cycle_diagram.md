# Scheduler Golden Diagram

## Purpose
Defines expected transitions between RUN and STOP  
and how scheduler decides next wake event.

---

# 1. Main Decision Loop

```
while (1):
    if (!joined):
        join_procedure()
        continue

    next_deadline = compute_next_deadline()

    enter_stop_until(next_deadline)

    wake()
    build_and_tx_uplink()
    process_downlink()
```

---

# 2. compute_next_deadline()
Uses:
- TDC (uplink interval)
- calibration ACK pending flag
- force-uplink flag
- join state
- radio availability

Example:
```
if (calibration_ack_pending)
    return now + 2s
if (force_uplink)
    return now + 1s
else
    return now + TDC
```

---

# 3. enter_stop_until()

- configure RTC  
- disable clocks  
- power down SX1276  
- execute STOP instruction  

---

# 4. wake()

- clocks ON  
- SX1276 regulator ON  
- SX1276 init  
- resume MAC  
- scheduler continues  

---

# 5. Failure Conditions

Scheduler must NOT:
- skip STOP  
- schedule next TX in the past  
- trigger double TX  
- ignore pending ACKs  
- get stuck waiting forever  

---

# Source of Truth
Matches logic in:
- `scheduler.c`
- `power.c`
- `uplink_encoder.c`
