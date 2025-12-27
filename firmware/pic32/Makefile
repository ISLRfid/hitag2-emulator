# Hi-Tag 2 Emulator - PIC32 Firmware Makefile
# For use with XC32 compiler

# Compiler settings
CC = xc32-gcc
AS = xc32-as
LD = xc32-ld
OBJCOPY = xc32-objcopy
OBJDUMP = xc32-objdump
SIZE = xc32-size

# Compiler flags
CFLAGS = -mprocessor=PIC32MX795F512L
CFLAGS += -O2 -Wall -Wextra
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -std=gnu99
CFLAGS += -I./include

# Assembler flags
ASFLAGS = -mprocessor=PIC32MX795F512L
ASFLAGS += -I./include

# Linker flags
LDFLAGS = -mprocessor=PIC32MX795F512L
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--defsym=_min_heap_size=1024
LDFLAGS += -T linker_script.ld

# Output files
TARGET = hitag2_emulator
HEX = $(TARGET).hex
ELF = $(TARGET).elf
MAP = $(TARGET).map

# Source files
SRC = src/main.c
SRC += src/rf_driver.c
SRC += src/crypto.c
SRC += src/memory.c
SRC += src/spi_slave.c
SRC += src/debug.c

# Object files
OBJ = $(SRC:.c=.o)

# Default target
all: $(HEX)

# Build ELF file
$(ELF): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

# Build hex file
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@
	$(OBJCOPY) -O binary $< $(TARGET).bin

# Assemble startup code
src/startup.o: src/startup.s
	$(AS) $(ASFLAGS) -o $@ $<

# Clean
clean:
	rm -f $(OBJ) $(ELF) $(HEX) $(TARGET).bin $(MAP)

# Debug build
debug: CFLAGS += -DDEBUG -g
debug: clean all

# Size report
sizes: $(ELF)
	$(SIZE) -A -d $<

# Disassembly
disasm: $(ELF)
	$(OBJDUMP) -d $< > $(TARGET).dis

# Upload (requires pic32prog or similar)
upload: $(HEX)
	pic32prog -d /dev/ttyUSB0 -b 115200 $(HEX)

.PHONY: all clean debug sizes disasm upload
