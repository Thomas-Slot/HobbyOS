
/*
 * HobbyOS - kernel.c
 *
 * Our first actual kernel implementation.
 */

#include <stdint.h>


/* --------------------------------------------------
   SCREEN CONFIGURATION
   -------------------------------------------------- */

/* Standard VGA text-mode dimensions */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* White text on a black background */
#define VGA_COLOR  0x0F


/* --------------------------------------------------
   VGA MEMORY
   -------------------------------------------------- */

/*
 * VGA text memory starts at physical address 0xB8000.
 *
 * Every screen character occupies 2 bytes:
 * - Character byte
 * - Color attribute byte
 */

static volatile uint16_t *const VGA_BUFFER =
    (volatile uint16_t *)0xB8000;


/* --------------------------------------------------
   CLEAR SCREEN
   -------------------------------------------------- */

static void clear_screen(void)
{
    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        VGA_BUFFER[i] =
            ((uint16_t)VGA_COLOR << 8) | (uint8_t)' ';
    }
}


/* --------------------------------------------------
   PRINT TEXT AT A SPECIFIC SCREEN POSITION
   -------------------------------------------------- */

static void print_at(uint32_t row,
                     uint32_t column,
                     const char *text)
{
    if (row >= VGA_HEIGHT)
        return;

    while (*text != '\0' && column < VGA_WIDTH)
    {
        uint32_t position = row * VGA_WIDTH + column;

        VGA_BUFFER[position] =
            ((uint16_t)VGA_COLOR << 8) | (uint8_t)*text;

        text++;
        column++;
    }
}


/* --------------------------------------------------
   KERNEL ENTRY FUNCTION
   -------------------------------------------------- */

void kernel_main(uint32_t multiboot_magic,
                 uint32_t multiboot_info_address)
{
    /*
     * We are not using the boot information structure yet.
     * We will examine it in a later milestone.
     */
    (void)multiboot_info_address;


    /* Clear the screen */
    clear_screen();


    /* Print our first messages */
    print_at(2, 4, "Welcome to HobbyOS!");

    print_at(5, 4, "Booted via GRUB Multiboot2 protocol");

    print_at(6, 4, "Kernel made in C is working");


    /* Verify that GRUB used the Multiboot2 protocol */
    if (multiboot_magic == 0x36D76289u)
    {
        print_at(7, 4, "Multiboot2 boot magic: VALID.");
    }
    else
    {
        print_at(7, 4, "WARNING: Unexpected boot magic.");
    }


    /* Show our current processor mode */
    print_at(8, 4, "Current execution mode: 32-bit protected mode.");


    /*
     * The kernel must not return to the Assembly startup code.
     * Keep the processor halted.
     */
    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}
