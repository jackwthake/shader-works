##############################################################################
BUILD = bin
BIN = software-rasterizer

##############################################################################
.PHONY: all directory clean size

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

LD_SCRIPT = linker/flash_with_bootloader.ld

ifeq ($(OS), Windows_NT)
  MKDIR = gmkdir
else
  MKDIR = mkdir
endif

CFLAGS += -W -Wall --std=gnu11 -Os
CFLAGS += -fno-diagnostics-show-caret
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -funsigned-char -funsigned-bitfields
CFLAGS += -mcpu=cortex-m4 -mthumb
CFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16
CFLAGS += -MD -MP -MT $(BUILD)/$(*F).o -MF $(BUILD)/$(@F).d

ASFLAGS += -mcpu=cortex-m4 -mthumb
ASFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16

LDFLAGS += -mcpu=cortex-m4 -mthumb
LDFLAGS += -mfloat-abi=softfp -mfpu=fpv4-sp-d16
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--script=$(LD_SCRIPT)

SRCDIR = src
INCDIR = include

INCLUDES += \
  -I$(INCDIR) \
  -I$(SRCDIR)

SRCS += \
  $(SRCDIR)/main.c \
  $(SRCDIR)/startup_samd51.c

ASRCS += \
  $(SRCDIR)/startup.s

DEFINES += \
  -D__SAMD51J19A__ \
  -DDONT_USE_CMSIS_INIT \
	'-DF_CPU=200000000L' \
	'-DARDUINO=10607' \
	 '-DARDUINO_PYGAMER_M4' \
	'-DARDUINO_ARCH_SAMD' \
	'-DARDUINO_SAMD_ADAFRUIT' \
	'-DCRYSTALLESS' \
	'-DADAFRUIT_PYGAMER_M4_EXPRESS' \
	'-D__SAMD51__' \
	'-D__FPU_PRESENT' \
	'-DARM_MATH_CM4' \
	'-mfloat-abi=soft' \
	'-mfpu=fpv4-sp-d16' \
	'-DUSB_VID=0x239A' \
	'-DUSB_PID=0x803D' \
	'-DUSBCON' \
	'-DUSB_CONFIG_POWER=100' \
	'-DUSB_MANUFACTURER="Adafruit"' \
	'-DUSB_PRODUCT="PyGamer M4 Express"'

CFLAGS += $(INCLUDES) $(DEFINES)
ASFLAGS += $(INCLUDES) $(DEFINES)

OBJS = $(addprefix $(BUILD)/, $(notdir $(subst .c,.o, $(SRCS))))
AOBJS = $(addprefix $(BUILD)/, $(notdir $(subst .s,.o, $(ASRCS))))

all: directory $(BUILD)/$(BIN).elf $(BUILD)/$(BIN).hex $(BUILD)/$(BIN).bin size

$(BUILD)/$(BIN).elf: $(OBJS) $(AOBJS)
	@echo LD $@
	@$(CC) $(LDFLAGS) $(OBJS) $(AOBJS) $(LIBS) -o $@

$(BUILD)/$(BIN).hex: $(BUILD)/$(BIN).elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O ihex $^ $@

$(BUILD)/$(BIN).bin: $(BUILD)/$(BIN).elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O binary $^ $@

$(BUILD)/%.o: $(SRCDIR)/%.c
	@echo CC $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SRCDIR)/%.s
	@echo AS $@
	@$(CC) $(ASFLAGS) -c $< -o $@

directory:
	@$(MKDIR) -p $(BUILD)

size: $(BUILD)/$(BIN).elf
	@echo size:
	@$(SIZE) -t $^

clean:
	@echo clean
	@-rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)

