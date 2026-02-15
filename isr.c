#include "isr.h"
#include "idt.h"
#include "pic.h"
#include "io.h"

isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler)
{
    interrupt_handlers[n] = handler;
}

extern void terminal_writestring(const char* data);

void isr_handler(registers_t *regs)
{
    if (interrupt_handlers[regs->int_no] != 0)
    {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
    else
    {
        if (regs->int_no < 32) {
            terminal_writestring("Received Exception\n");
            asm volatile("hlt");
        } else {
             terminal_writestring("Unhandled Interrupt\n");
        }
    }
}

void irq_handler(registers_t *regs)
{
    if (interrupt_handlers[regs->int_no] != 0)
    {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }

    if(regs->int_no >= 32)
        pic_send_eoi(regs->int_no - 32);
}
