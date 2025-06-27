###########################################################
# Directory variables
###########################################################
LIB_DIR=./lib
TOOLS_DIR=./tools
CORE_DIR=$(LIB_DIR)/samd-core/cores/arduino
BIN_DIR=./bin
OBJ_DIR=./bin/objects

## FIXME!! Pretty sure this isnt compiling all the files

###########################################################
# Compilers, Flags, Utility programs
###########################################################
CXX=$(TOOLS_DIR)/arm-none-eabi-gcc/9-2019q4/bin/arm-none-eabi-g++
CC=$(TOOLS_DIR)/arm-none-eabi-gcc/9-2019q4/bin/arm-none-eabi-gcc
AR=$(TOOLS_DIR)/arm-none-eabi-gcc/9-2019q4/bin/arm-none-eabi-ar

CXXFLAGS=-std=gnu++11 -mcpu=cortex-m4 -mthumb -ffunction-sections -fdata-sections -fno-threadsafe-statics -nostdlib --param max-inline-insns-single=500 -fno-rtti -fno-exceptions -MMD -Wall
CFLAGS=-std=gnu11 -mcpu=cortex-m4 -mthumb -ffunction-sections -fdata-sections --param max-inline-insns-single=500 -fno-exceptions -MMD -Wall
DEFINES=-D__SKETCH_NAME__="software-rasterizer.ino" -DF_CPU=200000000L -DARDUINO=10607 -DARDUINO_PYGAMER_M4 -DARDUINO_ARCH_SAMD -DARDUINO_SAMD_ADAFRUIT -D__SAMD51J19A__ -DCRYSTALLESS -DADAFRUIT_PYGAMER_M4_EXPRESS -D__SAMD51__ -D__FPU_PRESENT -DARM_MATH_CM4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DUSB_VID=0x239A -DUSB_PID=0x803D -DUSBCON -DUSB_CONFIG_POWER=100 -DUSB_MANUFACTURER="Adafruit" -DUSB_PRODUCT="PyGamer M4 Express"

###########################################################
# Include variables
###########################################################
CORE_INC=-I./lib/TinyUSB/src/ -I$(LIB_DIR)/CMSIS/CMSIS/Core/Include/ -I$(LIB_DIR)/DSP/Include/ -I$(LIB_DIR)/CMSIS-Atmel/CMSIS-Atmel/CMSIS/Device/ATMEL/ -I$(CORE_DIR) -I$(LIB_DIR)/variant/

###########################################################
# **Arduino Core Build**
# OBJ File variables (files to be built)
###########################################################
_USB_CORE_OBJ=CDC.o PluggableUSB.o USBCore.o
USB_CORE_OBJ=$(patsubst %,$(OBJ_DIR)/%,$(_USB_CORE_OBJ))

_MAIN_CORE_CXX_OBJ=IPAddress.o Print.o Reset.o SERCOM.o Stream.o Tone.o Uart.o WMath.o WString.o abi.o new.o
_MAIN_CORE_CC_OBJ=WInterrupts.o delay.o itoa.o pulse.o wiring.o wiring_digital.o wiring_shift.o cortex_handlers.o hooks.o math_helper.o startup.o wiring_analog.o wiring_private.o
MAIN_CORE_CXX_OBJ=$(patsubst %,$(OBJ_DIR)/%,$(_MAIN_CORE_CXX_OBJ))
MAIN_CORE_CC_OBJ=$(patsubst %,$(OBJ_DIR)/%,$(_MAIN_CORE_CC_OBJ))

all: core dependencies

###########################################################
# **Arduino and Board Core Compile**
###########################################################
core: avr_core arduino_core usb_core $(OBJ_DIR)/variant.o

avr_core: $(CORE_DIR)/avr/dtostrf.c
	$(CC) -c $(CFLAGS) $(CORE_INC) $(DEFINES) $< -o $(OBJ_DIR)/dtostrf.o

usb_core: $(USB_CORE_OBJ) $(OBJ_DIR)/samd21_host.o
	$(AR) rcs $(BIN_DIR)/core.a $^ $(OBJ_DIR)/samd21_host.o

arduino_core: $(MAIN_CORE_CXX_OBJ) $(MAIN_CORE_CC_OBJ)
	$(AR) rcs $(BIN_DIR)/core.a $^

$(USB_CORE_OBJ): $(CORE_DIR)/USB/*.cpp
	$(CC) -c $(CFLAGS) $(CORE_INC) $(DEFINES) $(CORE_DIR)/USB/samd21_host.c -o $(OBJ_DIR)/samd21_host.o
	$(CXX) -c $(CXXFLAGS) $(CORE_INC) $(DEFINES) $< -o $@

$(MAIN_CORE_CXX_OBJ): $(CORE_DIR)/*.cpp
	$(CXX) -c $(CXXFLAGS) $(CORE_INC) $(DEFINES) $< -o $@

$(MAIN_CORE_CC_OBJ): $(CORE_DIR)/*.c
	$(CC) -c $(CFLAGS) $(CORE_INC) $(DEFINES) $< -o $@

$(OBJ_DIR)/variant.o: $(LIB_DIR)/variant/variant.cpp
	$(CXX) -c $(CXXFLAGS) $(CORE_INC) $(DEFINES) $< -o $@
	$(AR) rcs $(BIN_DIR)/core.a $@

###########################################################
# ** Project Dependencies
###########################################################

lib_inc = $(CORE_INC) -I$(LIB_DIR)/Adafruit-GFX/ -I$(LIB_DIR)/BusIO/ -I$(LIB_DIR)/samd-core/libraries/Wire/ -I$(LIB_DIR)/samd-core/libraries/SPI/ -I$(LIB_DIR)/samd-core/libraries/Adafruit_ZeroDMA/

_ST7735_OBJ=Adafruit_ST7735.o Adafruit_ST77xx.o
ST7735_OBJ=$(patsubst %,$(OBJ_DIR)/%,$(_ST7735_OBJ))

_ADAFRUIT_GFX_OBJ=Adafruit_GFX.o Adafruit_SPITFT.o
ADAFRUIT_GFX_OBJ=$(patsubst %,$(OBJ_DIR)/%,$(_ADAFRUIT_GFX_OBJ))

dependencies: $(OBJ_DIR)/ST7735.o $(ADAFRUIT_GFX_OBJ)

# Display Library
$(OBJ_DIR)/ST7735.o: $(LIB_DIR)/ST7735/*.cpp
	$(CXX) -c $(CXXFLAGS) $(lib_inc) $(DEFINES) $< -o $@


$(ADAFRUIT_GFX_OBJ): $(LIB_DIR)/Adafruit-GFX/*.cpp
	$(CC) -c $(CFLAGS) $(lib_inc) $(DEFINES) $(LIB_DIR)/Adafruit-GFX/glcdfont.c -o $(OBJ_DIR)/glcdfont.o
	$(CXX) -c $(CXXFLAGS) $(lib_inc) $(DEFINES) $< -o $@
	$(AR) rcs $(BIN_DIR)/GFX.a $(ADAFRUIT_GFX_OBJ)


clean: $(BIN_DIR) $(OBJ_DIR)
	rm -f $(OBJ_DIR)/*
	rm -f $(BIN_DIR)/*.bin
	rm -f $(BIN_DIR)/*.elf
	rm -f $(BIN_DIR)/*.map
	rm -f $(BIN_DIR)/*.hex
	rm -f $(BIN_DIR)/*.a