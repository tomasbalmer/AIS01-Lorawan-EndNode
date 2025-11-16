# AT Command Specification  
## AIS01-LB Custom Firmware (Authoritative)

This document defines the FULL AT command surface implemented  
by the AIS01-LB custom firmware.  

Everything in `docs/legacy/` is OEM and NOT applicable.

---

# 1. General Format

```
AT+<CMD>[=<value>]
AT+<CMD>?
```

Responses:
- `OK`
- `<value>`
- `AT_ERROR`
- `AT_PARAM_ERROR`
- `AT_BUSY_ERROR`
- `AT_NO_NET_JOINED`

---

# 2. Core Commands

## 2.1 Ping
```
AT → OK
```

## 2.2 Version
```
AT+VER
→ ais01-fw-1.0.0-dev au915
```

---

# 3. Credentials (OTAA)

## DevEUI
```
AT+DEVEUI=<16 HEX>
AT+DEVEUI?
```

## AppEUI
```
AT+APPEUI=<16 HEX>
AT+APPEUI?
```

## AppKey
```
AT+APPKEY=<32 HEX>
AT+APPKEY?
```

## Join
```
AT+JOIN
```

States:
- success → `OK`
- busy → `AT_BUSY_ERROR`
- network not joined → `AT_NO_NET_JOINED` (status queries)

---

# 4. LoRaWAN Parameters

## ADR
```
AT+ADR=<0|1>
AT+ADR?
```

## Data Rate
```
AT+DR=<0-15>
AT+DR?
```

## TX Power
```
AT+TXP=<0-5>
AT+TXP?
```

## Application Port
```
AT+PORT=<1..223>
AT+PORT?
```

## TX Interval (ms)
```
AT+TDC=<ms>
AT+TDC?
```

---

# 5. Calibration

## Remote Calibration
```
AT+CALIBREMOTE=<hex-payload>
AT+CALIBREMOTE?
```

Behavior:
- validates payload  
- applies calibration to RAM  
- persists to NVMM  
- schedules calibration ACK uplink  

Errors:
- `AT_PARAM_ERROR`
- `AT_BUSY_ERROR`

---

# 6. System

### Reset
```
ATZ
```

### Unique ID
```
AT+UUID
```

### Optional: Battery Voltage
(only if implemented in source)
```
AT+BAT?
```

---

# 7. Extended / Debug

```
AT+DEBUG=<0|1>
```

Enables extra log output.

---

# 8. Error Table

| Error | Meaning |
|-------|---------|
| `AT_ERROR` | Unknown command |
| `AT_PARAM_ERROR` | Invalid parameter |
| `AT_BUSY_ERROR` | System busy (TX, RX or join) |
| `AT_NO_NET_JOINED` | Requires join |

---

# Source of Truth
This document mirrors the implementation in `src/app/atcmd.c`.
