
#include <stdint.h>
#include <stddef.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289U
#define MULTIBOOT2_TAG_FRAMEBUFFER  8U

#define COM1 0x3F8

/* ----------------------------------------
   Serial debugging
   ---------------------------------------- */

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_init(void)
{
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x01);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static void serial_write_char(char character)
{
    unsigned int timeout = 1000000;

    while ((inb(COM1 + 5) & 0x20U) == 0U && timeout > 0U)
        --timeout;

    outb(COM1, (uint8_t)character);
}

static void serial_write_string(const char *text)
{
    while (*text)
        serial_write_char(*text++);
}

static void serial_write_number(uint32_t number)
{
    char digits[10];
    unsigned int count = 0;

    do {
        digits[count++] = (char)('0' + number % 10U);
        number /= 10U;
    } while (number != 0U);

    while (count > 0)
        serial_write_char(digits[--count]);
}

__attribute__((noreturn))
static void halt_forever(void)
{
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

/* ----------------------------------------
   Multiboot2 structures
   ---------------------------------------- */

struct multiboot_info
{
    uint32_t total_size;
    uint32_t reserved;
};

struct multiboot_tag
{
    uint32_t type;
    uint32_t size;
};

/*
 * Multiboot2 framebuffer information tag.
 *
 * The RGB channel fields are present when
 * framebuffer_type == 1.
 */

struct multiboot_framebuffer_tag
{
    uint32_t type;
    uint32_t size;

    uint64_t address;

    uint32_t pitch;
    uint32_t width;
    uint32_t height;

    uint8_t bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;

    uint8_t red_position;
    uint8_t red_size;

    uint8_t green_position;
    uint8_t green_size;

    uint8_t blue_position;
    uint8_t blue_size;
};

/* ----------------------------------------
   Find framebuffer information
   ---------------------------------------- */

static const struct multiboot_framebuffer_tag *
find_framebuffer(uint64_t boot_info_address)
{
    if (boot_info_address == 0)
        return 0;

    const uint8_t *base =
        (const uint8_t *)(uintptr_t)boot_info_address;

    const struct multiboot_info *info =
        (const struct multiboot_info *)base;

    if (info->total_size < 16)
        return 0;

    uint32_t offset = 8;

    while ((uint64_t)offset + 8 <= info->total_size)
    {
        const struct multiboot_tag *tag =
            (const struct multiboot_tag *)(base + offset);

        if (tag->size < 8)
            return 0;

        if ((uint64_t)offset + tag->size > info->total_size)
            return 0;

        if (tag->type == 0)
            return 0;

        if (tag->type == MULTIBOOT2_TAG_FRAMEBUFFER)
        {
            const uint32_t minimum_size =
                (uint32_t)offsetof(
                    struct multiboot_framebuffer_tag,
                    blue_size
                ) + sizeof(uint8_t);

            if (tag->size < minimum_size)
                return 0;

            return (const struct multiboot_framebuffer_tag *)tag;
        }

        uint64_t next =
            ((uint64_t)offset + tag->size + 7U) & ~7ULL;

        if (next > info->total_size || next <= offset)
            return 0;

        offset = (uint32_t)next;
    }

    return 0;
}

/* ----------------------------------------
   Graphics
   ---------------------------------------- */

/*
 * Convert an 8-bit colour channel into
 * the number of bits used by the framebuffer.
 */

static uint32_t scale_channel(uint8_t value, uint8_t bits)
{
    uint32_t maximum = (1U << bits) - 1U;

    return ((uint32_t)value * maximum + 127U) / 255U;
}

/*
 * Convert RGB values into the pixel format
 * reported by Multiboot2.
 */

static uint32_t make_pixel(
    const struct multiboot_framebuffer_tag *fb,
    uint8_t red,
    uint8_t green,
    uint8_t blue)
{
    uint32_t pixel = 0;

    pixel |= scale_channel(red, fb->red_size)
             << fb->red_position;

    pixel |= scale_channel(green, fb->green_size)
             << fb->green_position;

    pixel |= scale_channel(blue, fb->blue_size)
             << fb->blue_position;

    return pixel;
}

/*
 * Draw a filled rectangle.
 *
 * Coordinates and dimensions are measured
 * in pixels.
 */

static void draw_rect(
    const struct multiboot_framebuffer_tag *fb,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint8_t red,
    uint8_t green,
    uint8_t blue)
{
    if (x >= fb->width || y >= fb->height)
        return;

    if (width > fb->width - x)
        width = fb->width - x;

    if (height > fb->height - y)
        height = fb->height - y;

    uint32_t pixel = make_pixel(fb, red, green, blue);

    for (uint32_t row_index = 0;
         row_index < height;
         ++row_index)
    {
        volatile uint32_t *row =
            (volatile uint32_t *)(uintptr_t)(
                fb->address +
                (uint64_t)(y + row_index) * fb->pitch +
                (uint64_t)x * 4U
            );

        for (uint32_t column = 0;
             column < width;
             ++column)
        {
            row[column] = pixel;
        }
    }
}

/* ----------------------------------------
   Framebuffer initialization and test
   ---------------------------------------- */

static int framebuffer_init(
    const struct multiboot_framebuffer_tag *fb)
{
    if (fb == 0)
        return 0;

    /* Initially support 32-bit direct RGB only. */
    if (fb->framebuffer_type != 1 || fb->bpp != 32)
        return 0;

    if (fb->address == 0 || fb->width == 0 || fb->height == 0)
        return 0;

    if ((uint64_t)fb->width * 4U > fb->pitch)
        return 0;

    if (fb->pitch % 4U != 0)
        return 0;

    if (fb->red_size == 0 || fb->red_size > 8 ||
        fb->green_size == 0 || fb->green_size > 8 ||
        fb->blue_size == 0 || fb->blue_size > 8)
        return 0;

    if ((uint16_t)fb->red_position + fb->red_size > 32 ||
        (uint16_t)fb->green_position + fb->green_size > 32 ||
        (uint16_t)fb->blue_position + fb->blue_size > 32)
        return 0;

    serial_write_string("Framebuffer found: ");
    serial_write_number(fb->width);
    serial_write_string(" x ");
    serial_write_number(fb->height);
    serial_write_string(", ");
    serial_write_number(fb->bpp);
    serial_write_string(" bpp\r\n");

    /* Dark background. */
    draw_rect(
        fb, 0, 0,
        fb->width, fb->height,
        20, 28, 45
    );

    /* Three coloured test rectangles. */
    uint32_t margin = fb->width / 10U;
    uint32_t gap = fb->width / 20U;
    uint32_t bar_width =
        (fb->width - 2U * margin - 2U * gap) / 3U;

    uint32_t bar_height = fb->height / 3U;
    uint32_t top = fb->height / 3U;

    draw_rect(
        fb, margin, top,
        bar_width, bar_height,
        255, 50, 50
    );

    draw_rect(
        fb, margin + bar_width + gap, top,
        bar_width, bar_height,
        50, 255, 50
    );

    draw_rect(
        fb, margin + 2U * (bar_width + gap), top,
        bar_width, bar_height,
        50, 100, 255
    );

    return 1;
}

/* ----------------------------------------
   Kernel entry point
   ---------------------------------------- */

__attribute__((noreturn))
void kernel_main(uint32_t boot_magic, uint64_t boot_info_address)
{
    serial_init();

    serial_write_string("\r\n");
    serial_write_string("HobbyOS: Entered x86-64 kernel.\r\n");

    if (boot_magic != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        serial_write_string("ERROR: Invalid Multiboot2 magic.\r\n");
        halt_forever();
    }

    serial_write_string("HobbyOS: Multiboot2 handoff verified.\r\n");

    const struct multiboot_framebuffer_tag *fb =
        find_framebuffer(boot_info_address);

    if (fb == 0)
    {
        serial_write_string("ERROR: Framebuffer tag not found.\r\n");
        halt_forever();
    }

    if (!framebuffer_init(fb))
    {
        serial_write_string("ERROR: Unsupported framebuffer format.\r\n");
        halt_forever();
    }

    serial_write_string("HobbyOS: Graphics test completed.\r\n");

    halt_forever();
}
