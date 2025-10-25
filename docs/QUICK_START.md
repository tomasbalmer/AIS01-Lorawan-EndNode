# Quick Start - Dragino AIS01-LB

Guía rápida de inicio en 5 pasos.

## Paso 1: Compilar

```bash
cd src/apps/dragino-ais01lb
make clean && make -j4
```

**Salida esperada:**
```
arm-none-eabi-gcc ... main.o
arm-none-eabi-gcc ... lorawan_app.o
...
   text    data     bss     dec     hex filename
  85432    2048    4096   91576   16598 build/dragino-ais01lb.elf
```

## Paso 2: Cargar Firmware

```bash
# Opción A: st-flash
make flash

# Opción B: STM32CubeProgrammer
STM32_Programmer_CLI -c port=SWD -w build/dragino-ais01lb.bin 0x08004000 -v -rst
```

## Paso 3: Conectar Serial

```bash
# Linux/Mac
minicom -D /dev/ttyUSB0 -b 115200

# Windows
# Usar PuTTY o Tera Term
# Configurar: COM_X, 115200 baud, 8N1
```

**Deberías ver:**
```
===================================
  Dragino AIS01-LB Firmware
  Version: 1.0.0
  Region: AU915 Sub-band 2
===================================

+READY
```

## Paso 4: Configurar Credenciales

Reemplaza con tus credenciales de TTN/ChirpStack:

```
AT+DEVEUI=<TU_DEVEUI>
AT+APPEUI=<TU_APPEUI>
AT+APPKEY=<TU_APPKEY>
```

Ejemplo:
```
AT+DEVEUI=70B3D57ED0012345
OK

AT+APPEUI=0000000000000001
OK

AT+APPKEY=2B7E151628AED2A6ABF7158809CF4F3C
OK
```

## Paso 5: Join y Empezar

```
AT+JOIN
Joining network...
OK

# Esperar ~10-30 segundos
Network joined successfully
```

**¡Listo!** El dispositivo ahora enviará uplinks automáticamente cada 60 segundos.

---

## Comandos Más Usados

| Comando | Uso |
|---------|-----|
| `AT` | Test de comunicación |
| `AT+VER` | Ver versión firmware |
| `AT+DEVEUI=<hex>` | Set DevEUI |
| `AT+APPEUI=<hex>` | Set AppEUI |
| `AT+APPKEY=<hex>` | Set AppKey |
| `AT+JOIN` | Unirse a red LoRaWAN |
| `AT+TDC=<ms>` | Intervalo de uplink (ms) |
| `AT+ADR=<0/1>` | Habilitar/deshabilitar ADR |
| `AT+DR=<0-5>` | Set data rate |
| `AT+BAT` | Ver nivel batería |
| `ATZ` | Reiniciar |
| `AT+FDR` | Factory reset |

---

## Verificar Funcionamiento

### 1. Ver uplinks en LoRaWAN Server

- Acceder a tu servidor LoRaWAN (TTN, ChirpStack, etc.)
- Ir a la sección de dispositivos
- Buscar tu DevEUI
- Ver "Live Data" o "Events"

**Deberías ver:**
- Evento de Join Accept
- Uplinks periódicos cada 60s
- Payload de 5 bytes

### 2. Ver logs en serial

```
Uplink sent (5 bytes)
TX Done: datarate=2, txPower=0
```

### 3. Enviar downlink de prueba

Desde tu servidor LoRaWAN:
- Puerto: 2
- Payload: `01` (1 byte)
- Confirmar que llega al dispositivo:

```
RX: port=2, rssi=-45, snr=10, size=1
Downlink received on port 2
```

---

## Troubleshooting Rápido

### No responde AT commands
- Verificar baudrate: 115200 8N1
- Probar con `\r\n` al final (Enter)

### No se une a la red
- Verificar credenciales correctas
- Verificar gateway AU915 Sub-band 2
- Revisar cobertura (mover cerca del gateway)

### Uplinks no llegan
- Verificar que está joined: ver logs "Network joined"
- Verificar gateway online en servidor LoRaWAN
- Aumentar TX power: `AT+TXP=0` (máximo)

### Consumo alto
- Verificar `LOW_POWER_MODE_ENABLED=1` en config.h
- Aumentar TDC: `AT+TDC=300000` (5 min)

---

## Configuración Recomendada para Producción

```
AT+ADR=1           # Habilitar ADR
AT+DR=2            # DR2 (SF10BW125)
AT+TXP=0           # Max TX power
AT+TDC=120000      # 2 minutos entre uplinks
AT+PNACKMD=0       # Unconfirmed (menor consumo)
AT+FREQBAND=2      # Sub-band 2 (AU915)
```

---

## Soporte

- **Documentación completa**: Ver [README.md](README.md)
- **Guía de pruebas**: Ver [TEST_GUIDE.md](TEST_GUIDE.md)
- **Comandos AT detallados**: Ver README.md sección "Comandos AT"

---

**¡Listo para producción en 5 minutos!**
