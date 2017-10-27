#include<sys/defs.h>
#ifndef _ASMIO_H
#define _ASMIO_H

void  enableinterupts(void);
void disableinterrupts(void);
void haltTillInterrupt(void);
 void io_wait(void);
 uint8_t inb(uint16_t port);
 void outb(uint16_t port, uint8_t val);
 uint32_t inl(uint16_t port);
 void outl(uint16_t port, uint32_t val);






#endif
