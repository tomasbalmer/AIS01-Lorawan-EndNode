# DOCUMENTACIÓN DE HARDWARE - AIS01-LB

## Documentos Disponibles ✅

### 1. Manual de Usuario Oficial
**Fuente:** http://wiki.dragino.com/xwiki/bin/view/Main/User%20Manual%20for%20LoRaWAN%20End%20Nodes/AIS01-LB--LoRaWAN_AI_Image_End_Node_User_Manual/

**Contenido:**
- Especificaciones completas (power, camera, LoRa)
- Configuración vía BLE/UART/LoRaWAN
- Procedimiento de actualización OTA
- AT commands reference
- Network integration guides

### 2. Pin Mappings (Inferidos y Validados)
**Fuente:** `src/board/board-config.h`

**Estado:** ✅ Validados (el firmware compila y los pin mappings son consistentes con el hardware)

### 3. Reverse Engineering
**Fuente:** `docs/AIS01_bin_analysis/` (Ghidra analysis)

**Contenido:**
- Análisis del firmware OEM (400+ funciones)
- String literals y mensajes
- Interrupt vector table
- NVM memory map
- AT command matrix

---

## Datasheets Componentes Estándar 📚

### **STM32L072CZ (MCU)** 🔴 CRÍTICO
**Descargar de:** https://www.st.com/en/microcontrollers-microprocessors/stm32l072cz.html

**Documentos necesarios:**
1. **Datasheet** (DS10182) - Specs eléctricos, pinout, características
2. **Reference Manual** (RM0377) - Registros, periféricos, arquitectura
3. **Programming Manual** (PM0223) - Cortex-M0+ instruction set
4. **Errata Sheet** (ES0206) - Bugs conocidos del silicon

**Para qué lo necesitas:**
- ✅ Configuración de periféricos (UART, SPI, RTC, ADC)
- ✅ Low-power modes (STOP, STANDBY)
- ✅ Clock tree configuration
- ✅ Flash memory layout y EEPROM emulation
- ✅ Interrupt priorities y NVIC

**Prioridad:** 🔴 ALTA (ya tienes lo básico funcionando, pero útil para optimización)

---

### **SX1276 (LoRa Radio)** 🔴 CRÍTICO
**Descargar de:** https://www.semtech.com/products/wireless-rf/lora-core/sx1276

**Documentos necesarios:**
1. **Datasheet** (SX1276/77/78/79) - Specs RF, registros, timings
2. **Application Notes:**
   - AN1200.13: SX1276/77/78/79 Datasheet
   - AN1200.22: LoRa Modem Designer's Guide
   - AN1200.24: LoRa Sensitivity Optimization

**Para qué lo necesitas:**
- ✅ Register configuration (ya tienes driver en `src/radio/sx1276/`)
- ✅ RF timing y calibración
- ✅ Antenna matching
- ✅ Power consumption optimization

**Prioridad:** 🟡 MEDIA (tu driver ya funciona, pero útil para troubleshooting RF)

---

### **OV2640 (Camera)** 🟡 OPCIONAL
**Descargar de:** https://www.ovt.com/ (o buscar en web)

**Documentos necesarios:**
1. **Datasheet** - Image sensor specs
2. **Application Notes** - JPEG compression, register settings

**Para qué lo necesitas:**
- ⚠️ Solo si vas a usar la cámara
- ⚠️ Protocolo en tu firmware es vía UART (no I2C directo)
- ⚠️ AI processor intermedio maneja la cámara

**Prioridad:** 🟢 BAJA (solo si necesitas cámara; protocolo UART ya parcialmente documentado)

---

## Documentación Faltante (No Disponible Públicamente) ⚠️

### 1. **Schematic Completo AIS01-LB**
**Estado:** ❌ No público (Dragino propietario)

**Alternativas:**
- Contactar Dragino directamente: support@dragino.com
- Inferir del código (ya hecho en `board-config.h`)
- Usar multímetro para tracing (si necesitas modificar hardware)

**¿Lo necesitas?**
- ❌ NO para firmware development (ya tienes pin mappings)
- ✅ SÍ si quieres diseñar tu propio hardware compatible
- ✅ SÍ si necesitas troubleshoot problemas RF/power

---

### 2. **Bootloader Dragino (0x08000000–0x08003FFF)**
**Estado:** ⚠️ Binario propietario, protocolo parcialmente documentado

**Lo que sabes:**
- OTA update vía Dragino Sensor Manager (UART)
- App offset: 0x08004000 (16 KB bootloader)
- Flash process: OK (documentado en wiki)

**Lo que falta:**
- Protocolo exacto UART para update
- Handshake sequence
- CRC/validation mechanism

**Prioridad:** 🟡 MEDIA
- Para usar update OTA básico: Ya está documentado en wiki
- Para implementar custom updater: Necesitas reverse engineer

**Acción recomendada:**
1. Usar herramienta oficial Dragino por ahora
2. Reverse engineer bootloader con Ghidra (8 horas)
3. O contactar Dragino para obtener protocolo

---

## Plan de Acción: Documentación

### 🔴 **FASE 1: Descargar AHORA** (1-2 horas)

```bash
# Crear directorio para datasheets
mkdir -p docs/datasheets

# Descargar (links directos o manualmente):
1. STM32L072CZ Datasheet (DS10182)
2. STM32L0x2 Reference Manual (RM0377)
3. SX1276 Datasheet
4. LoRa Modem Designer's Guide (AN1200.22)
```

**Por qué:**
- Necesarios para optimización de power management
- Útiles para troubleshooting
- Referencia para configuración avanzada de periféricos

---

### 🟡 **FASE 2: Contactar Dragino** (si necesitas)

**Email:** support@dragino.com

**Solicitar:**
1. ✅ Schematic AIS01-LB (útil pero no crítico)
2. ✅ Protocolo bootloader UART (para custom OTA)
3. ✅ AI processor UART protocol completo (si usas cámara)
4. ✅ BOM (Bill of Materials) si planeas fabricar custom hardware

**Justificación:**
> "Estamos desarrollando firmware custom para AIS01-LB y necesitamos
> documentación técnica adicional para optimización y troubleshooting.
> ¿Pueden compartir schematic y protocolo de bootloader bajo NDA si es necesario?"

---

### 🟢 **FASE 3: Reverse Engineering** (si Dragino no responde)

**Bootloader (8 horas):**
```bash
# Dump bootloader de device
st-flash read bootloader_dump.bin 0x08000000 0x4000

# Analizar en Ghidra
- Buscar UART handlers
- Identificar comando de update
- Documentar handshake sequence
```

**AI Processor UART Protocol (8 horas):**
```bash
# Capturar tráfico UART con logic analyzer
- Conectar LA a LPUART1 (PA_9/PA_10)
- Enviar comandos conocidos
- Documentar responses
- Crear state machine del protocolo
```

---

## Resumen: ¿Qué Necesitas Realmente?

### **Para continuar firmware development:** ⚠️ POCO FALTANTE
```
✅ Pin mappings: Los tienes (board-config.h)
✅ Build system: Funciona (Makefile)
✅ LoRaWAN stack: Implementado (lorawan.c)
✅ Power management: Básico funciona (power.c)

📥 DESCARGAR: STM32L072 reference manual (gratis, ST.com)
📥 DESCARGAR: SX1276 datasheet (gratis, Semtech)

🔍 OPCIONAL: Schematic completo (pedir a Dragino)
```

### **Para producción robusta:** ⚠️ NECESITAS MÁS TESTING QUE DOCS
```
La documentación que tienes es SUFICIENTE.

Lo que falta NO ES documentación, sino:
1. ✅ Watchdog implementation (4h coding)
2. ✅ OTA update mechanism (60h coding)
3. ✅ Unit tests (16h coding)
4. ✅ Hardware testing (1 week)
5. ✅ Field testing (1 month)

Docs adicionales son "nice to have", no bloqueantes.
```

---

## Recomendación Final 💡

### **Para TU caso (firmware custom en hardware Dragino):**

1. **DESCARGA AHORA** (1 hora):
   - STM32L072 Reference Manual (RM0377)
   - SX1276 Datasheet

2. **ENFÓCATE EN CÓDIGO** (2-3 semanas):
   - Watchdog (4h)
   - Testing hardware real (1 week)
   - OTA básico usando tool Dragino (2h setup)
   - Unit tests (16h)

3. **CONTACTA DRAGINO** (si tienes tiempo):
   - Email pidiendo schematic y bootloader protocol
   - Útil pero NO bloqueante

4. **DOCUMENTA lo que descubras** (ongoing):
   - Protocolo AI sensor (si lo usas)
   - Timing y comportamiento real
   - Edge cases y workarounds

### **NO GASTES TIEMPO EN:**
- ❌ Buscar/crear schematic completo (no lo necesitas)
- ❌ Diseñar hardware custom (usa Dragino)
- ❌ Reverse engineer cada registro del MCU (usa reference manual ST)

---

## Enlaces Útiles 🔗

### Datasheets Oficiales (Descargar gratis)
- **STM32L072CZ:** https://www.st.com/en/microcontrollers-microprocessors/stm32l072cz.html
- **SX1276:** https://www.semtech.com/products/wireless-rf/lora-core/sx1276
- **LoRaWAN Spec:** https://lora-alliance.org/resource_hub/lorawan-specification-v1-0-3/

### Dragino Resources
- **Wiki AIS01-LB:** http://wiki.dragino.com/xwiki/bin/view/Main/User%20Manual%20for%20LoRaWAN%20End%20Nodes/AIS01-LB--LoRaWAN_AI_Image_End_Node_User_Manual/
- **GitHub Decoders:** https://github.com/dragino/dragino-end-node-decoder/tree/main/AIS01
- **Support:** support@dragino.com

### Herramientas Análisis
- **STM32CubeMX:** https://www.st.com/en/development-tools/stm32cubemx.html (clock config, pin config)
- **Ghidra:** https://ghidra-sre.org/ (reverse engineering)

---

**CONCLUSIÓN:** Tienes 90% de la documentación que necesitas. Lo que falta son 2-3 datasheets gratuitos (1h download) y testing real en hardware (1 week). El resto es "nice to have" pero NO bloqueante para producción.
