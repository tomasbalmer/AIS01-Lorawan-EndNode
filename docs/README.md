# Dragino AIS01-LB Custom Firmware

Firmware personalizado para el nodo Dragino AIS01-LB basado en STM32L072CZ y radio SX1276, utilizando el stack LoRaMAC-node (Semtech) para LoRaWAN AU915 Clase A OTAA.

## Características

- **LoRaWAN Región**: AU915 (Sub-band 2)
- **Clase**: Clase A (OTAA)
- **MCU**: STM32L072CZ (Cortex-M0+)
- **Radio**: SX1276
- **Interfaz**: UART (AT commands + debug)
- **Bajo consumo**: STOP mode + RTC (<20 µA objetivo)
- **Calibración remota**: Vía AT commands y downlinks LoRaWAN
- **Persistencia**: Emulación EEPROM en flash

## Estructura del Proyecto

```
src/apps/dragino-ais01lb/
├── app/
│   ├── main.c              # State machine principal
│   ├── lorawan_app.c/h     # Integración con stack LoRaMAC
│   ├── atcmd.c/h           # Parser y handlers de comandos AT
│   ├── power.c/h           # Gestión de bajo consumo (STOP mode)
│   ├── storage.c/h         # Persistencia (EEPROM emulada)
│   ├── calibration.c/h     # Calibración remota
│   └── config.h            # Configuración general
├── board/
│   └── board-config.h      # Definiciones de pines
├── Makefile                # Build con arm-gcc
├── stm32l072xx_flash_app.ld  # Linker script (FLASH @ 0x08004000)
└── README.md               # Este archivo
```

## Compilación

### Requisitos

- **Toolchain**: arm-none-eabi-gcc (versión 10.3 o superior recomendada)
- **Make**: GNU Make
- **LoRaMac-node**: Este repositorio ya contiene el stack

### Pasos

```bash
cd src/apps/dragino-ais01lb
make clean
make -j4
```

Salida esperada:
- `build/dragino-ais01lb.elf`
- `build/dragino-ais01lb.bin`
- `build/dragino-ais01lb.hex`

## Carga del Firmware

### Usando st-flash (OpenOCD)

```bash
make flash
# O manualmente:
st-flash write build/dragino-ais01lb.bin 0x08004000
```

### Usando STM32CubeProgrammer

```bash
STM32_Programmer_CLI -c port=SWD -w build/dragino-ais01lb.bin 0x08004000 -v -rst
```

### Usando Dragino OTA Tool

1. Convertir `.bin` a formato Dragino OTA
2. Cargar vía herramienta Dragino OTA Tool según manual

**IMPORTANTE**: El firmware inicia en `0x08004000` (16KB offset) para preservar el bootloader.

## Comandos AT

### Comandos Básicos

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `AT` | Test | `AT` → `OK` |
| `AT+VER` | Versión firmware | `AT+VER` → `+VER: 1.0.0` |
| `ATZ` | Reset | `ATZ` |
| `AT+FDR` | Factory reset | `AT+FDR` → `OK` |

### Credenciales LoRaWAN

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `AT+DEVEUI` | Get/Set DevEUI | `AT+DEVEUI=0102030405060708` |
| `AT+APPEUI` | Get/Set AppEUI | `AT+APPEUI=0102030405060708` |
| `AT+APPKEY` | Get/Set AppKey | `AT+APPKEY=0102030405060708090A0B0C0D0E0F10` |
| `AT+JOIN` | Iniciar OTAA join | `AT+JOIN` |

### Configuración LoRaWAN

| Comando | Descripción | Valores | Ejemplo |
|---------|-------------|---------|---------|
| `AT+ADR` | ADR enable | 0=OFF, 1=ON | `AT+ADR=1` |
| `AT+DR` | Data rate | 0-5 | `AT+DR=2` |
| `AT+TXP` | TX power | 0-10 | `AT+TXP=0` |
| `AT+TDC` | TX duty cycle (ms) | ≥1000 | `AT+TDC=60000` |
| `AT+PORT` | Application port | 1-223 | `AT+PORT=2` |
| `AT+PNACKMD` | Confirmed mode | 0=Unconfirmed, 1=Confirmed | `AT+PNACKMD=0` |
| `AT+RX2DR` | RX2 data rate | DR_8 (AU915) | `AT+RX2DR=8` |
| `AT+RX2FQ` | RX2 frequency (Hz) | 923300000 | `AT+RX2FQ=923300000` |
| `AT+FREQBAND` | Sub-band | 1-9 | `AT+FREQBAND=2` |

### Sistema

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `AT+BAT` | Nivel de batería | `AT+BAT` → `+BAT: 3.25V (85%)` |
| `AT+DEBUG` | Debug enable | `AT+DEBUG=1` |

### Calibración Remota

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `AT+CALIBREMOTE` | Comando calibración | `AT+CALIBREMOTE=02000000C0` |

Formato payload: `<opcode><data>`

Opcodes:
- `0x01`: Reset calibración
- `0x02`: Set offset (4 bytes float)
- `0x03`: Set gain (4 bytes float)
- `0x04`: Set threshold (4 bytes float)
- `0x05`: Query calibración actual

## Flujo de Uso

### 1. Primer Encendido

```
+READY

AT
OK

AT+VER
+VER: 1.0.0
OK
```

### 2. Configurar Credenciales

```
AT+DEVEUI=70B3D57ED0012345
OK

AT+APPEUI=0000000000000001
OK

AT+APPKEY=2B7E151628AED2A6ABF7158809CF4F3C
OK
```

### 3. Unirse a la Red

```
AT+JOIN
Joining network...
OK

[Esperar mensajes de debug]
Network joined successfully
```

### 4. Configurar Parámetros

```
AT+TDC=120000
OK

AT+ADR=1
OK

AT+DR=2
OK
```

### 5. Verificar Funcionamiento

El dispositivo enviará uplinks automáticamente según `TDC` configurado.

```
Uplink sent (5 bytes)
RX: port=2, rssi=-45, snr=10, size=0
```

## Downlinks LoRaWAN

### Puerto de Calibración (Puerto 10)

Formato: `0xA0 <opcode> <data>`

Ejemplo para set offset = 1.5:
```
Hex: A0 02 00 00 C0 3F
     │  │  └─────────┘
     │  │      └─ float 1.5 (little-endian)
     │  └─ Opcode 0x02 (SET_OFFSET)
     └─ CALIBRATION_OPCODE (0xA0)
```

El dispositivo responde con ACK en el siguiente uplink.

## Consumo de Energía

### Modos de Operación

| Modo | Corriente | Notas |
|------|-----------|-------|
| RUN (TX) | ~120 mA | Durante transmisión LoRa |
| RUN (CPU) | ~1.5 mA | Procesamiento |
| STOP | <20 µA | Objetivo (RTC + flash standby) |

### Optimizaciones

1. **STOP mode** activado entre uplinks
2. **RTC wake-up** para TX periódico
3. **SX1276 sleep** cuando no TX/RX
4. **Clock gating** de periféricos no usados
5. **LSE** (32768 Hz) para RTC en vez de LSI

## Troubleshooting

### No compila

- Verificar que `arm-none-eabi-gcc` esté instalado: `arm-none-eabi-gcc --version`
- Verificar paths en `Makefile` apuntan correctamente al stack LoRaMac

### No se une a la red

- Verificar credenciales: `AT+DEVEUI`, `AT+APPEUI`, `AT+APPKEY`
- Verificar región AU915 en gateway
- Verificar sub-band 2 habilitado en gateway
- Revisar mensajes de debug vía UART

### Consumo alto

- Verificar `LOW_POWER_MODE_ENABLED=1` en `config.h`
- Medir corriente en STOP mode (sin TX)
- Verificar radio entra en sleep: debug `Radio.Sleep()`

### AT commands no responden

- Verificar baudrate UART: 115200 8N1
- Verificar `AT_ECHO_ENABLED=1` en `config.h`
- Probar con `\r\n` al final de comandos

## Modificaciones de Hardware

Si tu hardware Dragino AIS01-LB tiene pines diferentes, modifica:

`board/board-config.h`:
```c
#define RADIO_RESET     PC_0   // Cambiar según tu schematic
#define RADIO_DIO_0     PB_4
// ...
```

## Próximos Pasos

1. **Implementar sensor real**: Modificar `PrepareUplinkPayload()` en `main.c`
2. **Añadir UART secundario**: Para interfaz con sensor/AI (ya definido en config)
3. **Optimizar consumo**: Medir y ajustar STOP mode
4. **Pruebas de campo**: Validar alcance y autonomía

## Licencia

Este firmware utiliza el stack LoRaMac-node de Semtech (Revised BSD License).

## Soporte

Para consultas sobre el firmware personalizado, contactar al equipo de desarrollo.

---

**Versión**: 1.0.0
**Fecha**: 2025
**MCU**: STM32L072CZ
**Radio**: SX1276
**Región**: AU915
