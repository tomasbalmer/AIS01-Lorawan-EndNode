# Hardware Power Model  
## AIS01-LB Custom Firmware

This document defines the power behavior of the firmware:
- STOP mode  
- RUN mode  
- SX1276 power gating  
- wake logic  
- consumption expectations  

---

# 1. Power Objectives

- RUN: ~1–2 mA  
- TX: ~120 mA  
- STOP: **<20 µA** target  

---

# 2. STOP Mode Entry

Steps:
- save state  
- disable peripherals  
- stop SX1276 regulator  
- set RTC alarm  
- IWDG running  
- enter STOP  

---

# 3. STOP Mode Exit

Wake events:
- RTC  
- EXTI (if enabled)  

Exit sequence:
- re-enable clocks  
- re-enable regulator  
- reinitialize radio  
- resume scheduler  

---

# 4. Radio Power Gating

SX1276 is turned OFF between uplinks:
- reduces drain  
- reduces regulator 5V path  

Radio reinit occurs each wake.

---

# 5. Risks

- forgetting to disable a peripheral → STOP >80 µA  
- misconfigured RTC → device wakes early  
- non-reset SX1276 → TX timeout  

---

# Source of Truth
Matches `power.c`, `radio.c`, and STOP-mode HAL behavior.
