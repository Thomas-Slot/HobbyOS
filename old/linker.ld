
/*
 * HobbyOS - linker.ld
 *
 * Defines the structure and memory layout
 * of the final kernel executable.
 */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)

ENTRY(_start)

SECTIONS
{
    /* Our kernel starts at 1 MiB */
    . = 0x00100000;

    /* Multiboot2 header must be near the beginning */
    .multiboot ALIGN(8) :
    {
        KEEP(*(.multiboot))
    }

    /* Executable machine instructions */
    .text ALIGN(4K) :
    {
        *(.text)
        *(.text.*)
    }

    /* Read-only data, such as string literals */
    .rodata ALIGN(4K) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized global and static variables */
    .data ALIGN(4K) :
    {
        *(.data)
        *(.data.*)
    }

    /* Uninitialized global and static variables */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* Remove unnecessary metadata */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note)
        *(.note.*)
    }
}
