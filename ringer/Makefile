PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

ARCH_FLAGS := -DSTM32F4 -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

LIBOPENCM3_DIR := ./libopencm3

FREERTOS_DIR := ./freertos
FREERTOS_SRC := $(FREERTOS_DIR)/tasks.c \
                $(FREERTOS_DIR)/list.c \
                $(FREERTOS_DIR)/portable/GCC/ARM_CM4F/port.c \
                $(FREERTOS_DIR)/portable/MemMang/heap_1.c

CFLAGS  := -I./ -I./$(LIBOPENCM3_DIR)/include -I$(FREERTOS_DIR)/include \
           -I$(FREERTOS_DIR)/portable/GCC/ARM_CM4F \
           -c -Os -fno-common -ffunction-sections -fdata-sections $(ARCH_FLAGS)
LDFLAGS := -L./$(LIBOPENCM3_DIR)/lib -Tstm32.ld --static -nostartfiles -Wl,--gc-sections $(ARCH_FLAGS)
LDLIBS  += -Wl,--start-group -lc -lgcc -lnosys -lopencm3_stm32f4 -Wl,--end-group

FREERTOS_OBJ := $(FREERTOS_SRC:.c=.o)

.PHONY: all clean libopencm3

all: libopencm3 ringer.bin

ringer.bin: ringer.elf 
	$(OBJCOPY) -Obinary $< $@

ringer.elf: ringer.o $(FREERTOS_OBJ) stm32.ld
	$(LD) $(LDFLAGS) ringer.o $(FREERTOS_OBJ) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

libopencm3:
	$(MAKE) -s -C $(LIBOPENCM3_DIR)

clean:
	rm -f ringer.lst ringer.o ringer.elf ringer.lst ringer.bin FreeRTOSConfig.h.gch $(FREERTOS_OBJ) 

flash: ringer.bin
	st-flash write ringer.bin 0x08000000
