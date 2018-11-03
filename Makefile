TOOLCHAIN ?= arm-none-eabi-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCAHIN)ld
AR = $(TOOLCHAIN)ar
OBJCOPY = $(TOOLCHAIN)objcopy
OBJDUMP = $(TOOLCHAIN)objdump
GDB = $(TOOLCHAIN)gdb

LIBGCC = $(shell $(CC) -print-libgcc-file-name)


SOURCES = Demo/task3_main.c \
          Demo/startup.c \
          Demo/Drivers/rpi_gpio.c \
          Demo/Drivers/rpi_irq.c \
          Demo/Drivers/rpi_aux.c \
          Source/tasks.c \
          Source/list.c \
          Source/powf.c \
          Source/sqrt.c \
          Source/sqrtf.c \
          Source/portable/GCC/RaspberryPi/port.c \
          Source/portable/GCC/RaspberryPi/portISR.c \
          Source/portable/GCC/RaspberryPi/portASM.c \
          Source/portable/MemMang/heap_4.c \
          Source/printk.c \
          Source/queue.c \
          Source/assert.c \
          Source/string.c \

OBJECTS = $(patsubst %.c,build/%.o,$(SOURCES))

INCDIRS = Source/include Source/portable/GCC/RaspberryPi \
          Demo/Drivers Demo/

CFLAGS = -Wall $(addprefix -I ,$(INCDIRS))
CFLAGS += -D RPI2
CFLAGS += -march=armv7-a -mtune=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4

ASFLAGS += -march=armv7-a -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard

LDFLAGS = 

.PHONY: all clean

all: $(MOD_NAME)

$(MOD_NAME): $(OBJECTS)
	ld -shared $(LDFLAGS) $< -o $@ $(LIBGCC)

build/%.o: %.c
	mkdir -p $(dir $@)
	$(TOOLCHAIN)gcc -c $(CFLAGS) $< -o $@

build/%.o: %.s
	mkdir -p $(dir $@)
	$(TOOLCHAIN)as $(ASFLAGS) $< -o $@

all: kernel7.list kernel7.img kernel7.syms kernel7.hex
	$(TOOLCHAIN)size kernel7.elf

kernel7.img: kernel7.elf
	$(TOOLCHAIN)objcopy kernel7.elf -O binary $@

kernel7.list: kernel7.elf
	$(TOOLCHAIN)objdump -D -S  kernel7.elf > $@

kernel7.syms: kernel7.elf
	$(TOOLCHAIN)objdump -t kernel7.elf > $@

kernel7.hex : kernel7.elf
	$(TOOLCHAIN)objcopy kernel7.elf -O ihex $@

kernel7.elf: $(OBJECTS)
	$(TOOLCHAIN)ld $^ -static -Map kernel7.map -o $@ -T Demo/raspberrypi.ld $(LIBGCC)

clean:
	rm -f $(OBJECTS)
	rm -f kernel7.list kernel7.img kernel7.syms
	rm -f kernel7.elf kernel7.hex kernel7.map 
	rm -rf build

