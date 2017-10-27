#include<sys/defs.h>
#ifndef __PIC_H
#define __PIC_H
 void  enableinterupts(void);
void disableinterrupts(void);
void haltTillInterrupt(void);
uint16_t pic_get_irr(void);
uint16_t pic_get_isr(void);
void PIC_sendEOI(unsigned char irq);
void PIC_remap(int offset1, int offset2);
#endif

