/*!
 * \file      config.h
 *
 * \brief     Dragino AIS01-LB Firmware Configuration
 *
 * \details   Central configuration file for the Dragino AIS01-LB custom firmware.
 *            Defines region, sub-band, flash offsets, debug settings, and version.
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "lorawan_types.h"

/* ============================================================================
 * FIRMWARE VERSION
 * ========================================================================== */
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0
#define FIRMWARE_VERSION ((FIRMWARE_VERSION_MAJOR << 16) | \
                          (FIRMWARE_VERSION_MINOR << 8) |  \
                          FIRMWARE_VERSION_PATCH)

/* ============================================================================
 * LoRaWAN REGION CONFIGURATION
 * ========================================================================== */
#ifndef ACTIVE_REGION
#define ACTIVE_REGION LORAWAN_REGION_AU915
#endif

/* AU915 Sub-band Configuration (Sub-band 2: channels 8-15) */
#define LORAWAN_AU915_SUB_BAND 2

/* ============================================================================
 * LoRaWAN DEFAULT PARAMETERS
 * ========================================================================== */
#define LORAWAN_DEFAULT_CLASS LORAWAN_DEVICE_CLASS_A
#define LORAWAN_DEFAULT_ADR_STATE 1     /* 1 = ADR ON */
#define LORAWAN_DEFAULT_DATARATE 0      /* DR0 */
#define LORAWAN_DEFAULT_TX_POWER 0      /* Max EIRP */
#define LORAWAN_DEFAULT_CONFIRMED_MSG 0 /* 0 = unconfirmed */
#define LORAWAN_DEFAULT_APP_PORT 2
#define LORAWAN_DEFAULT_TDC 60000  /* 60 seconds in ms */
#define LORAWAN_DUTYCYCLE_ON false /* AU915 has no duty cycle */

/* RX2 Configuration (AU915 standard) */
#define LORAWAN_RX2_FREQUENCY 923300000 /* 923.3 MHz */
#define LORAWAN_RX2_DATARATE 8

/* RX Delays (milliseconds) */
#define LORAWAN_RX1_DELAY 1000
#define LORAWAN_RX2_DELAY 2000

/* Join RX Delays (milliseconds) */
#define LORAWAN_JOIN_RX1_DELAY 5000
#define LORAWAN_JOIN_RX2_DELAY 6000

/* Retry Configuration for Confirmed Messages */
#define LORAWAN_DEFAULT_RETRY_COUNT 3
#define LORAWAN_DEFAULT_RETRY_DELAY 1000  /* 1 second between retries */

/* ============================================================================
 * FLASH MEMORY CONFIGURATION
 * ========================================================================== */
/* Bootloader protection: Application starts at offset to preserve bootloader */
#define APP_FLASH_ORIGIN 0x08004000 /* 16KB offset */
#define APP_FLASH_SIZE (176 * 1024) /* 176KB for app */

/* EEPROM Emulation in Flash (last 8KB of flash) */
#define EEPROM_BASE_ADDRESS 0x08080000
#define EEPROM_SIZE (4 * 1024)
#define EEPROM_PAGE_SIZE 64

/* ============================================================================
 * POWER MANAGEMENT CONFIGURATION
 * ========================================================================== */
#define LOW_POWER_MODE_ENABLED 1
#define TARGET_STOP_CURRENT_UA 20 /* Target: <20 µA in STOP mode */

/* ============================================================================
 * DEBUG CONFIGURATION
 * ========================================================================== */
#define DEBUG_ENABLED 1   /* Set to 0 to disable debug prints */
#define AT_ECHO_ENABLED 1 /* Echo AT commands */

#if DEBUG_ENABLED
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

/* ============================================================================
 * UART CONFIGURATION
 * ========================================================================== */
#define AT_UART_BAUDRATE 115200
#define AT_UART_BUFFER_SIZE 256

/* Secondary UART for sensor/AI interface (future use) */
#define SENSOR_UART_ENABLED 0
#define SENSOR_UART_BAUDRATE 9600

/* ============================================================================
 * APPLICATION BUFFER SIZES
 * ========================================================================== */
#define LORAWAN_APP_DATA_BUFFER_MAX_SIZE 242
#define AT_CMD_MAX_LENGTH 128
#define AT_RESPONSE_MAX_LENGTH 256

/* ============================================================================
 * CALIBRATION CONFIGURATION
 * ========================================================================== */
#define CALIBRATION_DOWNLINK_PORT 10
#define CALIBRATION_OPCODE 0xA0

/* ============================================================================
 * SENSOR CONFIGURATION (Stub for future implementation)
 * ========================================================================== */
#define SENSOR_ENABLE_PIN_ENABLED 1
#define SENSOR_POWER_ON_DELAY_MS 100

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */
