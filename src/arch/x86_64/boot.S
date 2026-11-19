
/*
 * HobbyOS x86-64 entry point.
 * Loaded by GRUB through the Multiboot2 protocol.
 */

.section .multiboot,"a"
.balign 8

mb2_header_start:
    .long 0xE85250D6                  /* Multiboot2 magic */
    .long 0                           /* Architecture: i386 */
    .long mb2_header_end - mb2_header_start
    .long -(0xE85250D6 + (mb2_header_end - mb2_header_start))

    /* Request that EFI boot services remain available. */
    .short 7
    .short 0
    .long 8

    /* EFI AMD64 entry address. */
    .balign 8
    .short 9
    .short 0
    .long 12
    .long _start

    /* End tag. */
    /* Request a 1024x768, 32-bit framebuffer. */
    .balign 8
    .short 5
    .short 0
    .long 20
    .long 1024
    .long 768
    .long 32

mb2_header_end:


.section .text
.code64

.global _start
.type _start, @function

.extern kernel_main

_start:
    cli                         /* Disable CPU interrupts */
    cld                         /* Clear direction flag */

    /* Set up our own 16 KiB stack. */
    leaq stack_top(%rip), %rsp
    andq $-16, %rsp

    /* Pass Multiboot2 arguments to the C function. */
    movl %eax, %edi             /* Boot magic */
    movq %rbx, %rsi             /* Boot information pointer */

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

.size _start, . - _start


.section .bss
.balign 16

stack_bottom:
    .skip 16384

stack_top:
