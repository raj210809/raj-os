NASM       ?= nasm
QEMU       ?= qemu-system-x86_64
GDB        ?= gdb
CC         ?= gcc
LD         ?= ld
OBJCOPY    ?= objcopy

BUILD_DIR  := build
DISK       := $(BUILD_DIR)/disk.img
DISK_DEBUG := $(BUILD_DIR)/disk-debug.img

BOOT_SRC   := boot/boot.asm
KERNEL_ENTRY := kernel/entry.asm
KERNEL_LD  := kernel/linker.ld

BOOT_BIN   := $(BUILD_DIR)/boot.bin
BOOT_DEBUG := $(BUILD_DIR)/boot-debug.bin
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
BOOT_LST   := $(BUILD_DIR)/boot.lst
ADDRS_GDB  := debug/addresses.gdb

NASM_BOOT_FLAGS   := -f bin -I include -I boot
NASM_BOOT_DEBUG   := -f bin -I include -I boot -DDEBUG_SERIAL
NASM_KERNEL_FLAGS := -f elf64 -I include -I kernel

# Phases A–E: 64-bit kernel with E820/PMM, IDT, PIC/IRQ, and keyboard.
KERNEL_C_SRCS := \
	kernel/main.c \
	kernel/serial64.c \
	kernel/drivers/vga.c \
	kernel/mm/memmap.c \
	kernel/mm/pmm.c \
	kernel/mm/vmm.c \
	kernel/mm/vmm_test.c \
	kernel/mm/kmalloc.c \
	kernel/mm/kmalloc_test.c \
	kernel/arch/idt.c \
	kernel/arch/idt_tests.c \
	kernel/arch/irq.c \
	kernel/arch/pic.c \
	kernel/drivers/keyboard.c \
	kernel/drivers/pit.c
KERNEL_C_OBJS := $(KERNEL_C_SRCS:%.c=$(BUILD_DIR)/%.o)
KERNEL_ASM_OBJS := \
	$(BUILD_DIR)/kernel/longmode.o \
	$(BUILD_DIR)/kernel/entry.o \
	$(BUILD_DIR)/kernel/arch/idt_stubs.o

CFLAGS := -m64 -ffreestanding -fno-pie -fno-stack-protector -mno-red-zone \
          -nostdlib -Wall -Wextra -Werror -g -I include
LDFLAGS := -m elf_x86_64 -nostdlib -T $(KERNEL_LD)

KERNEL_SECTORS := 44
KERNEL_BYTES   := $(shell echo $$(( $(KERNEL_SECTORS) * 512 )))

QEMU_DRIVE   := -drive file=$(DISK),format=raw,if=ide,index=0,media=disk -boot c
QEMU_DRIVE_DEBUG := -drive file=$(DISK_DEBUG),format=raw,if=ide,index=0,media=disk -boot c
QEMU_SERIAL  := -serial stdio
QEMU_GDB     := -s -S
QEMU_MACHINE ?= -machine pc-i440fx-9.2
QEMU_VGA     ?= -vga std
QEMU_COMMON  := $(QEMU_MACHINE) $(QEMU_VGA)

EXERCISES_DIR := exercises
EX1_BOOT := $(EXERCISES_DIR)/exercise1.bin
EX2_BOOT := $(EXERCISES_DIR)/exercise2.bin
EX3_BOOT := $(EXERCISES_DIR)/exercise3.bin
EX3_DATA := $(EXERCISES_DIR)/test_data.bin
EX3_DISK := $(EXERCISES_DIR)/exercise3.img

.PHONY: all run gdb gdb-server gdb-connect debug clean \
	exercise1-run exercise2-run exercise3-run exercise3-gdb exercises-clean

all: $(DISK)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/kernel/drivers $(BUILD_DIR)/kernel/arch $(BUILD_DIR)/kernel/mm

$(BOOT_BIN): $(BOOT_SRC) boot/*.inc include/constants.inc | $(BUILD_DIR)
	$(NASM) $(NASM_BOOT_FLAGS) $(BOOT_SRC) -l $(BOOT_LST) -o $@
	@test $$(wc -c < $@) -eq 512

$(BUILD_DIR)/%.o: %.c include/*.h | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# GCC vectorizes PMM init loops into broken SIMD stores — build at -O0.
$(BUILD_DIR)/kernel/mm/pmm.o: kernel/mm/pmm.c include/*.h | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -O0 -c $< -o $@

$(BUILD_DIR)/kernel/mm/kmalloc.o: kernel/mm/kmalloc.c include/*.h | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -O0 -c $< -o $@

$(BUILD_DIR)/kernel/mm/kmalloc_test.o: kernel/mm/kmalloc_test.c include/*.h | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -O0 -c $< -o $@

$(BUILD_DIR)/kernel/mm/vmm.o: kernel/mm/vmm.c include/*.h | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -O0 -c $< -o $@

$(BUILD_DIR)/kernel/mm/vmm_test.o: kernel/mm/vmm_test.c include/*.h | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -O0 -c $< -o $@

$(BUILD_DIR)/kernel/longmode.o: kernel/longmode.asm include/constants.inc | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(NASM) $(NASM_KERNEL_FLAGS) kernel/longmode.asm -o $@

$(BUILD_DIR)/kernel/entry.o: $(KERNEL_ENTRY) include/constants.inc | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(NASM) $(NASM_KERNEL_FLAGS) $(KERNEL_ENTRY) -o $@

$(BUILD_DIR)/kernel/arch/idt_stubs.o: kernel/arch/idt_stubs.asm include/constants.inc | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(NASM) $(NASM_KERNEL_FLAGS) kernel/arch/idt_stubs.asm -o $@

$(KERNEL_ELF): $(KERNEL_LD) $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@
	@SIZE=$$(wc -c < $@); \
	if [ $$SIZE -gt $(KERNEL_BYTES) ]; then \
		echo "kernel.bin too large ($$SIZE > $(KERNEL_BYTES) bytes)"; exit 1; \
	fi
	@truncate -s $(KERNEL_BYTES) $@

$(DISK): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@

run: $(DISK)
	@echo "Serial log (boot + entry + kmain) -> this terminal. VGA -> QEMU window."
	$(QEMU) $(QEMU_COMMON) $(QEMU_DRIVE) $(QEMU_SERIAL) -display default

$(BOOT_DEBUG): $(BOOT_SRC) boot/*.inc include/constants.inc | $(BUILD_DIR)
	$(NASM) $(NASM_BOOT_DEBUG) $(BOOT_SRC) -l $(BOOT_LST) -o $@
	@test $$(wc -c < $@) -eq 512

$(DISK_DEBUG): $(BOOT_DEBUG) $(KERNEL_BIN)
	cat $(BOOT_DEBUG) $(KERNEL_BIN) > $@

$(ADDRS_GDB): $(BOOT_DEBUG) debug/gen-syms.sh
	@chmod +x debug/gen-syms.sh
	debug/gen-syms.sh $(BOOT_LST) > $@

gdb-server: $(DISK_DEBUG) $(KERNEL_ELF) $(ADDRS_GDB)
	@echo ""
	@echo "  QEMU running (paused). Serial output -> THIS terminal."
	@if [ "$(DEBUG_HEADLESS)" = "1" ]; then \
		echo "  Display: headless"; \
	else \
		echo "  Display: QEMU window (export DEBUG_HEADLESS=1 to hide)"; \
	fi
	@echo "  CPU is frozen until you connect GDB and type 'c'."
	@echo "  Open another terminal (nix develop) and run:"
	@echo "    make gdb-connect"
	@echo ""
	@if [ "$(DEBUG_HEADLESS)" = "1" ]; then \
		$(QEMU) $(QEMU_COMMON) $(QEMU_DRIVE_DEBUG) $(QEMU_SERIAL) $(QEMU_GDB) -display none; \
	else \
		$(QEMU) $(QEMU_COMMON) $(QEMU_DRIVE_DEBUG) $(QEMU_SERIAL) $(QEMU_GDB) -display default; \
	fi

gdb-connect: $(KERNEL_ELF) $(ADDRS_GDB)
	$(GDB) -x debug/kernel.gdb

debug: $(DISK_DEBUG) $(KERNEL_ELF) $(ADDRS_GDB)
	@chmod +x scripts/debug.sh
	./scripts/debug.sh

gdb: gdb-server

$(EX1_BOOT): $(EXERCISES_DIR)/exercise1.asm
	$(NASM) -f bin $< -o $@

$(EX2_BOOT): $(EXERCISES_DIR)/exercise2.asm
	$(NASM) -f bin $< -o $@

$(EX3_BOOT): $(EXERCISES_DIR)/exercise3.asm
	$(NASM) -f bin $< -o $@

$(EX3_DATA): $(EXERCISES_DIR)/test_data.asm
	$(NASM) -f bin $< -o $@

$(EX3_DISK): $(EX3_BOOT) $(EX3_DATA)
	cat $^ > $@

exercise1-run: $(EX1_BOOT)
	@echo "--- Exercise 1: segment registers ---"
	$(QEMU) -drive file=$(EX1_BOOT),format=raw,if=ide,index=0,media=disk -boot c

exercise2-run: $(EX2_BOOT)
	@echo "--- Exercise 2: direct VGA write ---"
	$(QEMU) -drive file=$(EX2_BOOT),format=raw,if=ide,index=0,media=disk -boot c

exercise3-run: $(EX3_DISK)
	@echo "--- Exercise 3: sector load ---"
	$(QEMU) -drive file=$(EX3_DISK),format=raw,if=ide,index=0,media=disk -boot c

exercise3-gdb: $(EX3_DISK)
	@echo "  Terminal 2: $(GDB) -ex 'target remote :1234' -ex 'c' -ex 'x/8x 0x10000'"
	$(QEMU) -drive file=$(EX3_DISK),format=raw,if=ide,index=0,media=disk -boot c -s -S

exercises-clean:
	rm -f $(EX1_BOOT) $(EX2_BOOT) $(EX3_BOOT) $(EX3_DATA) $(EX3_DISK)

clean: exercises-clean
	rm -rf $(BUILD_DIR)
	rm -f $(ADDRS_GDB)
