/* kernel.c */
#include <stdint.h>
#include <stddef.h>
#include "idt.h"
#include "gdt.h"
#include "pic.h"
#include "keyboard.h"

/* Hardware text mode color constants. */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
    return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) 
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) 
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) 0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) 
{
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) 
{
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
    }
}

void terminal_writestring(const char* data) 
{
    size_t datalen = strlen(data);
    for (size_t i = 0; i < datalen; i++)
        terminal_putchar(data[i]);
}

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

void disable_cursor(void)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void sleep(unsigned int mseconds)
{
    // A VERY approximate delay loop since we don't have timer interrupts yet
    // Adjusted speed as requested
    volatile unsigned long long count = 0;
    unsigned long long target = mseconds * 333333; 
    while (count < target) count++;
}

void terminal_clear(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
}

void type_text_centered(const char* text, size_t row, uint8_t color, unsigned int delay_ms)
{
    size_t len = strlen(text);
    size_t start_col = (VGA_WIDTH - len) / 2;
    terminal_row = row;
    terminal_column = start_col;
    terminal_setcolor(color);

    for (size_t i = 0; i < len; i++) {
        terminal_putchar(text[i]);
        sleep(delay_ms);
    }
}

void kernel_main(void) 
{
    terminal_initialize();
    disable_cursor(); // Hide the distracting hardware cursor
    
    // Initialize Interrupts
    gdt_init();
    idt_init();
    pic_remap(0x20, 0x28); // Map IRQ0-7 to 32-39, IRQ8-15 to 40-47
    keyboard_init();
    
    // Enable Interrupts
    asm volatile("sti");

    // Initial boot sequence feel
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("Booting Piper OS Kernel...\n");
    sleep(500);
    terminal_writestring("Initializing memory manager... [OK]\n");
    sleep(300);
    terminal_writestring("Loading drivers... [OK]\n");
    sleep(300);
    terminal_writestring("Establishing uplink... [OK]\n");
    sleep(800);
    
    // Clear screen for the main reveal
    terminal_clear();
    sleep(500);

    // ASCII Art Animation: PiperOS
    const char* logo[] = {
        "  ____  _                  ___  ____  ",
        " |  _ \\(_)_ __   ___ _ __ / _ \\/ ___| ",
        " | |_) | | '_ \\ / _ \\ '__| | | \\___ \\ ",
        " |  __/| | |_) |  __/ |  | |_| |___) |",
        " |_|   |_| .__/ \\___|_|   \\___/|____/ ",
        "         |_|                          "
    };
    
    size_t logo_height = sizeof(logo) / sizeof(logo[0]);
    size_t start_row = (VGA_HEIGHT - logo_height) / 2 - 2;

    for (size_t i = 0; i < logo_height; i++) {
        type_text_centered(logo[i], start_row + i, vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), 10);
    }

    sleep(1000);
    
    const char* welcome = "Welcome to the new OS";
    type_text_centered(welcome, start_row + logo_height + 2, vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 50);
    
    // Blinking cursor effect at the end
    terminal_row = start_row + logo_height + 4;
    terminal_column = (VGA_WIDTH / 2) - 1;
    terminal_writestring("> ");
    
    while(1) {
        terminal_putentryat('_', vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), terminal_column, terminal_row);
        sleep(500);
        terminal_putentryat(' ', vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), terminal_column, terminal_row);
        sleep(500);
    }
}
