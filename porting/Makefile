PAYLOADS=vm.c
#PAYLOADS=$(wildcard *.c)
# TARGETS=$(patsubst %.c,%.bin,$(PAYLOADS))
TARGETS=vm.bin

ASM=$(wildcard lib/*.S)
SRC=$(wildcard lib/*.c)

OBJ = $(ASM:.S=.o) $(SRC:.c=.o)

PREFIX=riscv64-elf
CC=$(PREFIX)-gcc
AS=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OBJDUMP=$(PREFIX)-objdump
OBJCOPY=$(PREFIX)-objcopy

OPT ?= 1

# COMMON_FLAGS=-MMD -ffreestanding -nostdlib -nostartfiles -march=rv64imafc -mabi=lp64
COMMON_FLAGS=-MMD -ffreestanding -nostdlib -nostartfiles -march=rv64gc -mabi=lp64
CFLAGS=$(COMMON_FLAGS) -O$(OPT) -Ilib
CFLAGS += -falign-functions=4
ASFLAGS=$(COMMON_FLAGS) -Ilib
LDFLAGS=-nostdlib -flto

all: tags $(TARGETS)

clean:
	rm -f *.o *.elf *.bin *.d *.list tags

tags: $(wildcard *.[chS])
	ctags *.[chS]

ifneq ($(MAKECMDGOALS),clean)
-include $(wildcard *.d)
endif

%.bin: %.elf %.list
	$(OBJCOPY) $< -O binary $@
%.list: %.elf
	$(OBJDUMP) $< -D > $@

%.elf: memmap.ld %.o $(OBJ)
	$(LD) $(LDFLAGS) -T $^ -o $@

%.o: %.S
	$(AS) -c $< $(ASFLAGS) -o $@
%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

.PHONY: all clean
.PRECIOUS: %.list
