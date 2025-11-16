# AT Command Specification  
## AIS01-LB Custom Firmware

This document defines the **complete, authoritative AT command surface**  
implemented by the custom AIS01-LB firmware.

OEM AT commands **DO NOT APPLY** unless explicitly documented.

---

# 1. Command Format

All commands follow:

```
AT+<CMD>[=<value>]
```

Responses:
- `OK`
- `AT_ERROR`
- `AT_PARAM_ERROR`
- `AT_BUSY_ERROR`
- `AT_NO_NET_JOINED`
- `<value>` (when querying)

---

# 2. Core Commands

## 2.1 Ping
```
AT
→ OK
```

## 2.2 Version
```
AT+VER
→ ais01-fw-1.0.0-dev au915
```

---

# 3. LoRaWAN Credentials

## 3.1 Device EUI
```
AT+DEVEUI=<16 hex>
AT+DEVEUI?
```

## 3.2 Application EUI
```
AT+APPEUI=<16 hex>
AT+APPEUI?
```

## 3.3 Application Key
```
AT+APPKEY=<32 hex>
AT+APPKEY?
```

## 3.4 Join
```
AT+JOIN
```

Triggers OTAA join.  
Errors:
- `AT_BUSY_ERROR`
- `AT_NO_NET_JOINED` (when querying status)

---

# 4. Session / Network Parameters

## 4.1 ADR
```
AT+ADR=<0|1>
AT+ADR?
```

## 4.2 Data Rate
```
AT+DR=<0-15>
AT+DR?
```

## 4.3 TX Power
```
AT+TXP=<0-5>
AT+TXP?
```

## 4.4 Port
```
AT+PORT=<1-223>
AT+PORT?
```

## 4.5 Uplink interval (ms)
```
AT+TDC=<ms>
AT+TDC?
```

---

# 5. Calibration

## 5.1 Remote Calibration Command
```
AT+CALIBREMOTE=<hex>
AT+CALIBREMOTE?
```

- Validates the calibration payload  
- Writes to NVMM  
- Schedules a calibration ACK uplink  

---

# 6. System Commands

## 6.1 Reset
```
ATZ
```

## 6.2 Read Unique ID
```
AT+UUID
```

## 6.3 Read battery voltage
(optional if implemented in your code)
```
AT+BAT?
```

---

# 7. Optional / Debug

## 7.1 Debug mode
```
AT+DEBUG=<0|1>
```

Prints extended logs.

---

# 8. Errors

| Error | Meaning |
|--------|---------|
| `AT_ERROR` | Unknown command |
| `AT_PARAM_ERROR` | bad parameter |
| `AT_BUSY_ERROR` | System busy (TX/RX or join in progress) |
| `AT_NO_NET_JOINED` | Action requires join first |

---

# 9. Source of Truth

This document corresponds directly to:  
`src/app/atcmd.c`

Any command not listed here is considered unsupported.
