#ifndef PIC_H
#define PIC_H

#include <stdint.h>

void pic_remap(int offset1, int offset2);
void pic_send_eoi(unsigned char irq);
void irq_set_mask(unsigned char irq_line);
void irq_clear_mask(unsigned char irq_line);

#endif
