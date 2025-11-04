##############################################################################
# Makefile for Dragino AIS01-LB Firmware
# Target: STM32L072CZ + SX1276 (AU915)
# Toolchain: arm-none-eabi-gcc
##############################################################################

# Project name
PROJECT = ais01

# Build directory
BUILD_DIR = build

# Toolchain
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

# MCU configuration
MCU = -mcpu=cortex-m0plus -mthumb
FPU =
FLOAT-ABI =

# Defines
DEFS = -DSTM32L072xx -DUSE_HAL_DRIVER -DACTIVE_REGION=LORAMAC_REGION_AU915

# Paths
SRC_DIR = src
BOARD_DIR = $(SRC_DIR)/board
CMSIS_DIR = $(SRC_DIR)/cmsis
RADIO_DIR = $(SRC_DIR)/radio
SYSTEM_DIR = $(SRC_DIR)/system
LORAWAN_DIR = $(SRC_DIR)/lorawan

# Include paths
INCLUDES = \
-I$(SRC_DIR)/app \
-I$(BOARD_DIR) \
-I$(CMSIS_DIR) \
-I$(RADIO_DIR) \
-I$(RADIO_DIR)/sx1276 \
-I$(SYSTEM_DIR) \
-I$(LORAWAN_DIR)

# Application sources
APP_SOURCES = \
$(SRC_DIR)/app/main.c \
$(SRC_DIR)/app/storage.c \
$(SRC_DIR)/app/calibration.c \
$(SRC_DIR)/app/power.c \
$(SRC_DIR)/app/atcmd.c \
$(SRC_DIR)/app/lorawan_app.c

# Board sources
BOARD_SOURCES = \
$(BOARD_DIR)/board.c \
$(BOARD_DIR)/gpio-board.c \
$(BOARD_DIR)/spi-board.c \
$(BOARD_DIR)/uart-board.c \
$(BOARD_DIR)/rtc-board.c \
$(BOARD_DIR)/adc-board.c \
$(BOARD_DIR)/delay-board.c \
$(BOARD_DIR)/sx1276-board.c \
$(BOARD_DIR)/eeprom-board.c \
$(BOARD_DIR)/lpm-board.c \
$(BOARD_DIR)/sysIrqHandlers.c

# CMSIS sources
CMSIS_SOURCES = \
$(CMSIS_DIR)/system_stm32l0xx.c

# Minimal LoRaWAN stack sources
LORAWAN_SOURCES = \
$(LORAWAN_DIR)/lorawan.c \
$(LORAWAN_DIR)/lorawan_crypto.c \
$(LORAWAN_DIR)/lorawan_region.c \
$(LORAWAN_DIR)/lorawan_region_au915.c \
$(LORAWAN_DIR)/aes.c \
$(LORAWAN_DIR)/cmac.c

# Radio sources
RADIO_SOURCES = \
$(RADIO_DIR)/sx1276/sx1276.c

# System sources
SYSTEM_SOURCES = \
$(SYSTEM_DIR)/delay.c \
$(SYSTEM_DIR)/gpio.c \
$(SYSTEM_DIR)/timer.c \
$(SYSTEM_DIR)/systime.c \
$(SYSTEM_DIR)/nvmm.c \
$(SYSTEM_DIR)/fifo.c \
$(SYSTEM_DIR)/adc.c \
$(SYSTEM_DIR)/uart.c \
$(SYSTEM_DIR)/hal_stubs.c \
$(SYSTEM_DIR)/syscalls.c \
$(SYSTEM_DIR)/utilities.c

# Startup file
ASM_SOURCES = \
$(CMSIS_DIR)/arm-gcc/startup_stm32l072xx.s

# All sources
SOURCES = $(APP_SOURCES) $(BOARD_SOURCES) $(CMSIS_SOURCES) \
          $(LORAWAN_SOURCES) $(RADIO_SOURCES) $(SYSTEM_SOURCES)

# Objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Compiler flags
CFLAGS = $(MCU) $(DEFS) $(INCLUDES) -Wall -fdata-sections -ffunction-sections
CFLAGS += -O2 -g3

# Linker script
LDSCRIPT = stm32l072xx_flash_app.ld

# Linker flags
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections

# Default target
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin

# Build .elf
$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

# Build .hex
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(CP) -O ihex $< $@

# Build .bin
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(CP) -O binary -S $< $@

# Build objects from C sources
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

# Build objects from ASM sources
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

# Create build directory
$(BUILD_DIR):
	mkdir $@

# Clean
clean:
	-rm -fR $(BUILD_DIR)

# Flash (using STM32_Programmer_CLI or st-flash)
flash: all
	@echo "Flash using: st-flash write $(BUILD_DIR)/$(PROJECT).bin 0x08004000"
	st-flash write $(BUILD_DIR)/$(PROJECT).bin 0x08004000

.PHONY: all clean flash
