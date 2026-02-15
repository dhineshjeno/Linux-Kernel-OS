#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// IDT Entry Structure
struct idt_entry_struct
{
    uint16_t base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
    uint16_t sel;                 // Kernel segment selector.
    uint8_t  always0;             // This must always be zero.
    uint8_t  flags;               // More flags. See documentation.
    uint16_t base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));

typedef struct idt_entry_struct idt_entry_t;

// IDT Pointer Structure
struct idt_ptr_struct
{
    uint16_t limit;
    uint32_t base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));

typedef struct idt_ptr_struct idt_ptr_t;

// Initialization function
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif
