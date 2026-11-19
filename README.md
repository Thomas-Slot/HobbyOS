My very own HobbyOS developed with the guidance of AI.



Linux Arch (tested on Fish, bash not tested yet.) build instructions:

1. The following packages need to be installed on the system:
    - base-devel
    - grub
    - xorriso
    - libisoburn
    - mtools
    - dosfstools
    - git (optional)
    - i686-elf-binutils (yay -S)
    - i686-elf-gcc (yay -S)
    - gcc and g++
    - nasm

2. Execute the following commands in a terminal located in the directory where all the files find themselves:
    1. i686-elf-gcc -m32 -c src/boot.S -o build/boot.o
    2. i686-elf-gcc \
        -m32 \
        -std=gnu11 \
        -O2 \
        -Wall \
        -Wextra \
        -ffreestanding \
        -fno-stack-protector \
        -no-pic \
        -fno-pie \
        -fno-unwind-tables \
        -fno-asynchronous-unwind-tables \
        -mno-mmx \
        -mno-sse \
        -mno-sse2 \
        -mno-80387 \
        -c src/kernel.c \
        -o build/kernel.o
    3. i686-elf-gcc \
        -m32 \
        -T linker.ld \
        -o build/kernel.elf \
        -ffreestanding \
        -O2 \
        -nostdlib \
        -fno-pie \
        -no-pie \
        -Wl,--build-id=none \
        build/boot.o \
        build/kernel.o \
        -lgcc
    4. cp build/kernel.elf iso/boot/kernel.elf
    5. grub-mkrescue -o HobbyOS.iso iso

    What each command does:
    1. Assemble the boot.S file
    2. Compile the C kernel.
    3. Link the kernel via linked.ld
    4. Copy the built ELF kernel file into the ISO directory
    5. Make the bootable ISO file.

3. If you want to run it in Oracle VirtualBox (or any VM) make sure the following is true:
    1. 32-bit "Other" type of OS
    2. Correct ISO selected
    3. 32MiB of RAM is mimimum (with upcoming features)
    4. There doesn't have to be any diskspace
    5. 1 CPU is enough. Multi-core/thread implementations haven't been made.