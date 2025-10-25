# Arquitectura del Firmware - Dragino AIS01-LB

## Visión General

El firmware está diseñado como una máquina de estados modular para un nodo LoRaWAN Clase A con ultra-bajo consumo.

```
┌─────────────────────────────────────────────────────────────┐
│                     APLICACIÓN                               │
│  ┌────────────┐  ┌──────────┐  ┌────────────┐              │
│  │   main.c   │──│ atcmd.c  │──│ storage.c  │              │
│  │ (State     │  │ (AT CMD  │  │ (EEPROM    │              │
│  │  Machine)  │  │  Parser) │  │  Emul)     │              │
│  └─────┬──────┘  └────┬─────┘  └─────┬──────┘              │
│        │              │               │                      │
│  ┌─────▼──────────────▼───────────────▼──────┐              │
│  │         lorawan_app.c                     │              │
│  │  (LoRaWAN Application Layer)              │              │
│  └───────────────────┬───────────────────────┘              │
└────────────────────────┼─────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│              STACK LoRaMAC (Semtech)                         │
│  ┌──────────┐  ┌───────────┐  ┌─────────┐                  │
│  │ LoRaMac  │──│ Region    │──│ Crypto  │                  │
│  │ Handler  │  │ AU915     │  │         │                  │
│  └─────┬────┘  └───────────┘  └─────────┘                  │
└────────┼──────────────────────────────────────────────────────┘
         │
┌────────▼──────────────────────────────────────────────────────┐
│              HAL / BOARD SUPPORT                              │
│  ┌─────────┐  ┌──────────┐  ┌────────┐  ┌────────┐          │
│  │ Radio   │  │ UART     │  │ RTC    │  │ Power  │          │
│  │ SX1276  │  │ HAL      │  │ Timer  │  │ Mgmt   │          │
│  └─────────┘  └──────────┘  └────────┘  └────────┘          │
└───────────────────────────────────────────────────────────────┘
```

## Módulos Principales

### 1. main.c - State Machine

**Responsabilidad**: Orquestación del ciclo de vida de la aplicación

**Estados**:
```c
APP_STATE_BOOT      // Inicialización de módulos
APP_STATE_JOIN      // Proceso OTAA join
APP_STATE_IDLE      // Espera de eventos
APP_STATE_UPLINK    // Envío de datos
APP_STATE_RX        // Recepción downlinks
APP_STATE_SLEEP     // Modo bajo consumo
```

**Flujo**:
```
BOOT → JOIN → IDLE ⇄ UPLINK → SLEEP → IDLE
                ↑________________________│
                   (RTC Wake-up)
```

### 2. lorawan_app.c - LoRaWAN Application Layer

**Responsabilidad**: Abstracción del stack LoRaMAC

**Funciones clave**:
- `LoRaWANApp_Init()`: Configura región AU915, callbacks
- `LoRaWANApp_Join()`: Inicia OTAA join
- `LoRaWANApp_SendUplink()`: Envía datos al servidor
- `LoRaWANApp_Process()`: Procesa eventos del stack

**Callbacks implementados**:
- `OnJoinRequest()`: Maneja resultado del join
- `OnTxData()`: Confirmación de TX
- `OnRxData()`: Recepción de downlinks
- `OnMacMcpsRequest()`: Eventos MCPS
- `OnMacMlmeRequest()`: Eventos MLME

### 3. atcmd.c - AT Command Parser

**Responsabilidad**: Interface UART para configuración

**Arquitectura**:
```c
typedef struct {
    const char *name;
    ATCmdHandler_t handler;
    const char *help;
} ATCmdEntry_t;

static const ATCmdEntry_t g_ATCmdTable[] = {
    { "AT+DEVEUI", ATCmd_HandleDevEUI, "..." },
    { "AT+JOIN", ATCmd_HandleJoin, "..." },
    // ...
};
```

**Flujo de procesamiento**:
```
UART RX → ProcessChar() → Buffer → Parse() → Lookup Table → Handler()
                                                               ↓
                                                           Response()
```

### 4. storage.c - Non-Volatile Storage

**Responsabilidad**: Persistencia de configuración en flash

**Estructura de datos**:
```c
typedef struct {
    uint32_t Magic;              // 0xDEADBEEF
    uint32_t Version;            // 1
    StorageData_t Data;          // Configuración
    uint32_t Crc;                // CRC32
} StorageBlock_t;
```

**Memoria**:
- Base: `0x0802E000` (últimos 8KB de flash)
- Emulación EEPROM usando HAL Flash
- Write: Erase page → Write word-by-word
- Read: Direct memory access

### 5. calibration.c - Remote Calibration

**Responsabilidad**: Ajuste remoto de parámetros de sensor

**Comandos soportados**:
```c
CALIB_CMD_RESET        = 0x01  // Reset a defaults
CALIB_CMD_SET_OFFSET   = 0x02  // Ajustar offset
CALIB_CMD_SET_GAIN     = 0x03  // Ajustar ganancia
CALIB_CMD_SET_THRESHOLD= 0x04  // Ajustar threshold
CALIB_CMD_QUERY        = 0x05  // Consultar valores
```

**Canales de entrada**:
1. AT command: `AT+CALIBREMOTE=<payload>`
2. LoRaWAN downlink: Puerto 10, Opcode 0xA0

### 6. power.c - Power Management

**Responsabilidad**: Ultra-bajo consumo

**Modos**:
```c
POWER_MODE_RUN    // ~1.5 mA (CPU activa)
POWER_MODE_SLEEP  // ~500 µA (WFI)
POWER_MODE_STOP   // <20 µA (RTC + flash standby)
```

**Flujo de STOP mode**:
```
1. Disable peripherals (UART, GPIO clocks)
2. Put SX1276 in sleep
3. Configure RTC wake-up
4. Enter STOP mode (WFI)
5. [HARDWARE WAKEUP]
6. Restore system clock (HSI)
7. Re-enable peripherals
8. Continue execution
```

## Integración con LoRaMAC Stack

### Region AU915 Configuration

```c
// Sub-band 2 (channels 8-15)
uint16_t channelMask[] = {0x0000, 0x00FF, 0x0000, 0x0000, 0x0000};
//                        CH 0-15  CH 8-15 enabled
```

### Callbacks del Stack

```
LoRaMac Stack                  lorawan_app.c
─────────────                  ─────────────
LoRaMac_Init()        ←────    LoRaWANApp_Init()
LoRaMac_Join()        ←────    LoRaWANApp_Join()
LoRaMac_Send()        ←────    LoRaWANApp_SendUplink()

[CALLBACKS]
McpsConfirm()         ────→    OnMacMcpsRequest()
McpsIndication()      ────→    OnRxData()
MlmeConfirm()         ────→    OnMacMlmeRequest()
```

## Memory Map

```
FLASH (192 KB):
├─ 0x08000000 - 0x08003FFF  (16 KB)  Bootloader (preservado)
├─ 0x08004000 - 0x0802DFFF  (168 KB) Application code
└─ 0x0802E000 - 0x0802FFFF  (8 KB)   EEPROM emulation

RAM (20 KB):
├─ 0x20000000 - 0x20003FFF  (16 KB)  Variables + Stack
└─ 0x20004000 - 0x20004FFF  (4 KB)   Heap
```

## Secure Boot Considerations

El firmware inicia en `0x08004000` para preservar el bootloader original de Dragino:

**Linker Script**:
```ld
MEMORY {
  FLASH (rx) : ORIGIN = 0x08004000, LENGTH = 176K
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 20K
}
```

**Vector Table Relocation**:
```c
// En system_stm32l0xx.c o startup code
#define VECT_TAB_OFFSET  0x4000
SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
```

## Timing Constraints (LoRaWAN)

```
Join Accept:   RX1 = 5s,  RX2 = 6s    (después de Join Request)
Class A RX:    RX1 = 1s,  RX2 = 2s    (después de Uplink)
Duty Cycle:    AU915 no tiene restricción
ADR:           Adaptación dinámica de DR
```

## Power Budget

```
Evento              Duración    Corriente    Energía
─────────────────────────────────────────────────────
STOP mode           59s         15 µA        0.885 mAh
Wake-up + Process   100ms       1.5 mA       0.0042 mAh
LoRa TX (DR2)       300ms       120 mA       10 mAh
RX1 Window          100ms       15 mA        0.42 mAh
RX2 Window          100ms       15 mA        0.42 mAh
─────────────────────────────────────────────────────
TOTAL (60s cycle):                           11.7 mAh/ciclo

Con batería 2400 mAh:
Autonomía = 2400 / (11.7 * 1440) ≈ 14 días (ciclo 60s)
```

**Optimizaciones**:
- Aumentar TDC a 300s → Autonomía ~60 días
- Disable confirmed messages → -20% consumo
- Usar ADR para DR mayor → TX más corto

## Thread Safety

**Single-threaded architecture**:
- No RTOS
- Main loop + interrupts
- Critical sections con `CRITICAL_SECTION_BEGIN/END`

**Interrupciones**:
```c
RTC_WKUP_IRQHandler()    // Wake-up from STOP
USART2_IRQHandler()      // UART RX
DIO0_IRQHandler()        // Radio TX done
DIO1_IRQHandler()        // Radio RX timeout
```

## Error Handling

**Strategy**: Fail-safe y retry automático

```
Join Failed      → Retry con backoff exponencial
TX Failed        → Re-queue en siguiente ciclo
Storage CRC Fail → Load defaults, reinitialize
Radio Error      → Reset radio, reinitialize
Watchdog Timeout → System reset
```

## Debug Hooks

```c
// config.h
#define DEBUG_ENABLED  1        // Enable printf via UART
#define USE_RADIO_DEBUG        // Debug pins TX/RX toggle

// main.c
DEBUG_PRINT("Uplink sent: %d bytes\r\n", size);
```

**Production**:
- `DEBUG_ENABLED = 0` para menor tamaño de código
- Logs solo para errores críticos

## Future Extensions

1. **Sensor Interface**:
   - Modificar `PrepareUplinkPayload()` en `main.c`
   - Añadir `sensor.c/h` module
   - UART secundario ya definido en `board-config.h`

2. **FOTA (Firmware Over The Air)**:
   - Implementar downlink handler para firmware chunks
   - Usar bootloader existente de Dragino

3. **Class B/C**:
   - Modificar `LORAWAN_DEFAULT_CLASS` en `config.h`
   - Implementar beacon tracking

4. **Multi-region**:
   - Añadir detección automática de región
   - Compilar con `-DACTIVE_REGION=LORAMAC_REGION_US915`

---

**Notas de Implementación**:

- Código sigue estilo del stack LoRaMac-node de Semtech
- Compatible con LoRaWAN 1.0.3
- Tested con TTN y ChirpStack
- Cumple con regulaciones AU915 (ACMA)
