# Golden Power Cycle — STOP → WAKE → UPLINK

## Purpose
Defines the expected behavior of the device across its low-power cycle.

This is used to validate:
- STOP current (< 20 µA)
- correct RTC wake scheduling
- SX1276 reinitialization
- scheduler correctness
- no double-TX
- no hang states

---

# 1. Cycle Overview

```
INIT
→ JOIN
→ IDLE
→ STOP (RTC)
→ WAKE
→ UPLINK
→ IDLE
→ STOP
(repeat)
```

---

# 2. STOP Mode Requirements

In STOP:
- CPU halted  
- SRAM retained  
- RTC ON  
- SX1276 OFF  
- All peripheral clocks OFF  
- IWDG running  
- consumption target: **<20 µA**

---

# 3. Wake Conditions

Wake must occur **only by**:
- RTC alarm  
- EXTI sources (if enabled explicitly)

Wake must **NOT** occur from:
- spurious pending IRQs  
- radio interrupts (radio is off)  
- SysTick (disabled in STOP)

---

# 4. Expected Wake Sequence

Upon wake:
1. Re-enable system clocks  
2. Reconfigure GPIO  
3. Enable SX1276 regulator  
4. Reinitialize SX1276  
5. Restore LoRaMAC context  
6. Resume scheduler

---

# 5. Scheduler Timing Rules

The next UPLINK must occur:
- after `(now + TDC)` if idle  
- immediately if `force uplink` flag set  
- immediately after calibration ACK scheduling  
- only after JOIN success  
- must NOT slip into double scheduling  

---

# 6. Expected Timing Diagram

```
Time →
┌────────────┐   ┌────────────┐
│   UPLINK   │   │   UPLINK   │
└──────┬─────┘   └──────┬─────┘
       │                │
 STOP  │                │
<------┘                └------->
(Deep sleep with SX1276 off)
```

---

# 7. Edge Case Behavior

### Case: JOIN pending  
- Device must retry join  
- Must NOT enter STOP until first join window

### Case: Downlink inside RX window  
- Process downlink  
- Scheduler recomputes next-stop deadline  
- STOP only after handling downlink

### Case: Calibration ACK scheduled  
- Next UPLINK MUST include ACK  
- STOP skipped until ACK sent

---

# 8. Failure Modes (must not happen)

- STOP current > 30 µA  
- WAKE without RTC/EXTI  
- TX twice in a row without STOP  
- RTC drift > 5 seconds / hour  
- SX1276 not reinitialized after STOP  
- Join procedure restarting unexpectedly  

---

# Source of Truth
Matches behavior in:
- `scheduler.c`
- `power.c`
- `radio.c`
