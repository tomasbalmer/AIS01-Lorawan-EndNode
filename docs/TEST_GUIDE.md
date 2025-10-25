# Guía de Pruebas - Dragino AIS01-LB

Esta guía proporciona un conjunto completo de pruebas para validar el firmware del Dragino AIS01-LB.

## Pre-requisitos

1. Hardware conectado:
   - Dragino AIS01-LB con firmware cargado
   - Cable USB-TTL (UART @ 115200 baud)
   - Alimentación (batería o USB)

2. Software:
   - Terminal serial (minicom, PuTTY, screen, etc.)
   - Gateway LoRaWAN configurado para AU915 Sub-band 2
   - Servidor LoRaWAN (TTN, ChirpStack, etc.)

3. Credenciales LoRaWAN:
   - DevEUI
   - AppEUI (JoinEUI)
   - AppKey

## Test 1: Comunicación UART y Comandos Básicos

### Objetivo
Verificar que el dispositivo responde a comandos AT básicos.

### Procedimiento

```bash
# Conectar terminal serial a 115200 baud
minicom -D /dev/ttyUSB0 -b 115200

# Al encender el dispositivo, deberías ver:
```

**Salida esperada:**
```
===================================
  Dragino AIS01-LB Firmware
  Version: 1.0.0
  Region: AU915 Sub-band 2
===================================
Initialization complete

+READY
```

### Comandos de prueba

```
AT
OK

AT+VER
+VER: 1.0.0
OK

AT+BAT
+BAT: 3.30V (95%)
OK
```

**Estado**: ✅ PASS si responde correctamente

---

## Test 2: Configuración de Credenciales LoRaWAN

### Objetivo
Configurar y persistir credenciales LoRaWAN.

### Procedimiento

```
# 1. Configurar DevEUI
AT+DEVEUI=70B3D57ED0012345
OK

# 2. Verificar DevEUI
AT+DEVEUI
+DEVEUI: 70B3D57ED0012345
OK

# 3. Configurar AppEUI
AT+APPEUI=0000000000000001
OK

# 4. Configurar AppKey
AT+APPKEY=2B7E151628AED2A6ABF7158809CF4F3C
OK

# 5. Reiniciar para verificar persistencia
ATZ
Resetting...

# 6. Después del reset, verificar credenciales
AT+DEVEUI
+DEVEUI: 70B3D57ED0012345
OK
```

**Estado**: ✅ PASS si las credenciales persisten después del reset

---

## Test 3: Join OTAA

### Objetivo
Verificar que el dispositivo puede unirse a la red LoRaWAN via OTAA.

### Procedimiento

```
AT+JOIN
Joining network...
OK
```

**Mensajes de debug esperados:**
```
Initiating OTAA join...
MLME Request: status=0
Join successful!
Network joined successfully
```

**En el servidor LoRaWAN:**
- Verificar que aparece un evento de "Join Accept"
- Anotar el DevAddr asignado

**Estado**: ✅ PASS si join exitoso en <30 segundos

### Troubleshooting

Si el join falla:

1. Verificar gateway conectado
2. Verificar región AU915 en gateway
3. Verificar sub-band 2 habilitado
4. Revisar credenciales en servidor LoRaWAN
5. Verificar cobertura (RSSI > -120 dBm)

---

## Test 4: Uplink Manual

### Objetivo
Enviar un uplink manual después del join.

### Procedimiento

```
# Configurar parámetros de uplink
AT+TDC=60000
OK

AT+PORT=2
OK

AT+PNACKMD=0
OK

# El dispositivo enviará uplinks automáticamente
```

**Mensajes de debug esperados:**
```
Uplink sent (5 bytes)
TX Done: datarate=2, txPower=0
```

**En el servidor LoRaWAN:**
- Verificar recepción de uplink en puerto 2
- Verificar payload (5 bytes: battery + temperatura + sensor)
- Anotar RSSI y SNR

**Estado**: ✅ PASS si uplink recibido correctamente

---

## Test 5: Downlink y RX Windows

### Objetivo
Verificar recepción de downlinks en ventanas RX1/RX2.

### Procedimiento

1. Esperar a que el dispositivo envíe un uplink
2. Desde el servidor LoRaWAN, enviar un downlink:
   - Puerto: 2
   - Payload: `01` (ejemplo)

**Mensajes de debug esperados:**
```
RX: port=2, rssi=-45, snr=10, size=1
Downlink received on port 2
```

**Estado**: ✅ PASS si downlink recibido

---

## Test 6: Calibración Remota

### Objetivo
Probar comandos de calibración via AT y downlink.

### Procedimiento A: Via AT Command

```
# Reset calibración
AT+CALIBREMOTE=01
Calibration command processed
OK

# Set offset = 1.5 (float 0x3FC00000 en little-endian)
AT+CALIBREMOTE=020000C03F
Calibration command processed
OK

# Query calibración
AT+CALIBREMOTE=05
Calibration command processed
OK
```

**Mensajes de debug:**
```
Calibration reset via downlink
Calibration offset set to 1.50
```

### Procedimiento B: Via Downlink LoRaWAN

1. Enviar downlink en puerto 10:
   - Payload: `A0020000C03F` (Set offset = 1.5)

**Mensajes de debug:**
```
Processing calibration command
Calibration offset set to 1.50
```

2. El dispositivo responderá con un uplink de confirmación

**Estado**: ✅ PASS si calibración aplicada correctamente

---

## Test 7: Configuración de Parámetros LoRaWAN

### Objetivo
Verificar configuración de parámetros de red.

### Procedimiento

```
# ADR
AT+ADR=1
OK

AT+ADR
+ADR: 1
OK

# Data Rate
AT+DR=2
OK

AT+DR
+DR: 2
OK

# TX Power
AT+TXP=0
OK

AT+TXP
+TXP: 0
OK

# TX Duty Cycle
AT+TDC=120000
OK

AT+TDC
+TDC: 120000
OK

# Confirmed messages
AT+PNACKMD=1
OK

# RX2 configuration
AT+RX2DR=8
OK

AT+RX2FQ=923300000
OK

# Frequency band
AT+FREQBAND=2
OK
```

**Estado**: ✅ PASS si todos los parámetros se configuran y leen correctamente

---

## Test 8: Bajo Consumo (Low Power Mode)

### Objetivo
Verificar consumo en modo STOP.

### Equipo necesario
- Amperímetro de precisión (µA range)

### Procedimiento

1. Configurar TDC largo: `AT+TDC=300000` (5 min)
2. Esperar a que el dispositivo entre en STOP mode
3. Medir corriente

**Consumo esperado:**
- En STOP mode: **<20 µA**
- Durante uplink (TX): ~120 mA (breve)
- En RUN mode (CPU): ~1.5 mA

**Estado**: ✅ PASS si consumo en STOP <20 µA

### Debug de bajo consumo

Modificar `config.h`:
```c
#define LOW_POWER_MODE_ENABLED  0  // Disable para debug
```

Recompilar y observar si el problema persiste.

---

## Test 9: Factory Reset

### Objetivo
Verificar que el factory reset borra configuración.

### Procedimiento

```
# 1. Configurar credenciales
AT+DEVEUI=70B3D57ED0012345
OK

# 2. Factory reset
AT+FDR
Factory reset successful
OK

# 3. Verificar que se borró
AT+DEVEUI
+DEVEUI: 0000000000000000
OK
```

**Estado**: ✅ PASS si credenciales borradas

---

## Test 10: Reset y Watchdog

### Objetivo
Verificar comportamiento de reset.

### Procedimiento

```
# Software reset
ATZ
Resetting...

# Después del reset:
+READY
```

**Estado**: ✅ PASS si reinicia correctamente

---

## Test 11: Stress Test - Uplinks Continuos

### Objetivo
Verificar estabilidad con uplinks frecuentes.

### Procedimiento

```
# Configurar TDC corto
AT+TDC=10000
OK

# Dejar corriendo por 1 hora
# Observar logs
```

**Métricas esperadas:**
- 100% de uplinks enviados
- 0 errores de stack LoRaMAC
- Sin resets inesperados

**Estado**: ✅ PASS si estable durante 1 hora

---

## Test 12: Cobertura y Alcance

### Objetivo
Probar alcance en diferentes condiciones.

### Procedimiento

1. Test en línea de vista
2. Test en interior
3. Test en sótano/parking

**Métricas a registrar:**
- RSSI (dBm)
- SNR (dB)
- Tasa de éxito de uplinks
- Distancia al gateway

**Estado**: ✅ PASS según requerimientos del proyecto

---

## Checklist Final

Antes de deployment en producción:

- [ ] Test 1: UART y comandos básicos ✅
- [ ] Test 2: Configuración credenciales ✅
- [ ] Test 3: Join OTAA ✅
- [ ] Test 4: Uplink manual ✅
- [ ] Test 5: Downlink RX ✅
- [ ] Test 6: Calibración remota ✅
- [ ] Test 7: Configuración parámetros ✅
- [ ] Test 8: Bajo consumo <20µA ✅
- [ ] Test 9: Factory reset ✅
- [ ] Test 10: Reset y watchdog ✅
- [ ] Test 11: Stress test 1 hora ✅
- [ ] Test 12: Cobertura en campo ✅

---

## Logs de Debug

Para habilitar logs detallados, modificar `config.h`:

```c
#define DEBUG_ENABLED   1
```

Ejemplo de logs completos:

```
===================================
  Dragino AIS01-LB Firmware
  Version: 1.0.0
  Region: AU915 Sub-band 2
===================================
Storage initialized
Calibration initialized: Offset=0.00, Gain=1.00, Threshold=100.00
Power management initialized
LoRaWAN initialized (AU915, Sub-band 2)
Initialization complete
Starting OTAA join...
Initiating OTAA join...
MLME Request: status=0, nextTx=0
Join successful!
Network joined successfully
Uplink queued (port 2, size 5)
TX Done: datarate=2, txPower=0
Entering STOP mode for 60000 ms
Woke up from STOP mode (source: 1)
Uplink queued (port 2, size 5)
RX: port=2, rssi=-45, snr=10, size=1
Downlink received on port 2
```

---

## Anexo: Script de Test Automático

```python
#!/usr/bin/env python3
import serial
import time

def send_at_command(ser, cmd, wait=1):
    ser.write((cmd + '\r\n').encode())
    time.sleep(wait)
    response = ser.read(ser.in_waiting).decode()
    print(f">>> {cmd}")
    print(response)
    return response

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

# Test sequence
send_at_command(ser, "AT")
send_at_command(ser, "AT+VER")
send_at_command(ser, "AT+BAT")
send_at_command(ser, "AT+DEVEUI=70B3D57ED0012345")
send_at_command(ser, "AT+DEVEUI")

ser.close()
```

Guardar como `test_at.py` y ejecutar:
```bash
python3 test_at.py
```

---

**Fin de la Guía de Pruebas**
