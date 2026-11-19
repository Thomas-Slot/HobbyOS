
CC := x86_64-elf-gcc

BUILD_DIR := build/uefi
ISO_ROOT := $(BUILD_DIR)/iso-root

CFLAGS := \
    -std=gnu11 \
    -O2 \
    -Wall \
    -Wextra \
    -ffreestanding \
    -fno-builtin \
    -fno-stack-protector \
    -fno-pic \
    -fno-pie \
    -fno-unwind-tables \
    -fno-asynchronous-unwind-tables \
    -m64 \
    -mno-red-zone \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mno-80387

LDFLAGS := \
    -nostdlib \
    -no-pie \
    -Wl,--build-id=none \
    -Wl,-z,max-page-size=0x1000

OBJECTS := \
    $(BUILD_DIR)/boot.o \
    $(BUILD_DIR)/kernel.o

.PHONY: all kernel check iso clean

all: iso

kernel: $(BUILD_DIR)/kernel.elf

$(BUILD_DIR)/boot.o: src/arch/x86_64/boot.S
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: src/kernel/kernel.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.elf: $(OBJECTS) linker-x86_64.ld
	$(CC) $(CFLAGS) $(LDFLAGS) \
		-T linker-x86_64.ld \
		-o $@ $(OBJECTS) -lgcc

check: kernel
	grub-file --is-x86-multiboot2 $(BUILD_DIR)/kernel.elf
	@echo "Multiboot2 kernel verification passed."

iso: check
	rm -rf $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(BUILD_DIR)/kernel.elf $(ISO_ROOT)/boot/kernel.elf
	cp grub-uefi.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	rm -f $(BUILD_DIR)/HobbyOS-UEFI.iso
	grub-mkrescue -o $(BUILD_DIR)/HobbyOS-UEFI.iso $(ISO_ROOT)

clean:
	rm -rf $(BUILD_DIR)
