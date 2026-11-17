
/*
 * HobbyOS - boot.S
 *
 * This file contains:
 * 1. The Multiboot2 header.
 * 2. Our initial kernel stack.
 * 3. The kernel entry point.
 */


/* --------------------------------------------------
   PART 1: MULTIBOOT2 HEADER
   -------------------------------------------------- */

.section .multiboot, "a"
.align 8

mb2_header_start:

    .long 0xE85250D6
    .long 0
    .long mb2_header_end - mb2_header_start

    .long -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))

    /* Required end tag */
    .short 0
    .short 0
    .long 8

mb2_header_end:


/* --------------------------------------------------
   PART 2: KERNEL STACK
   -------------------------------------------------- */

.section .bss
.align 16

stack_bottom:
    .skip 16384

stack_top:


/* --------------------------------------------------
   PART 3: KERNEL ENTRY POINT
   -------------------------------------------------- */

.section .text

.global _start
.type _start, @function

.extern kernel_main

_start:

    /* Disable interrupts while we initialize */
    cli

    /* Clear the processor's direction flag */
    cld

    /* Set up our own stack */
    mov $stack_top, %esp

    /* Align stack to a 16-byte boundary */
    and $-16, %esp

    /* Reserve space for stack alignment */
    sub $8, %esp

    /* Pass Multiboot2 information to C */
    push %ebx
    push %eax

    /* Call our C kernel */
    call kernel_main

    /* Clean up stack if kernel_main returns */
    add $16, %esp


/* Kernel should never return */
halt_loop:

    cli
    hlt
    jmp halt_loop


.section .note.GNU-stack,"",@progbits
