#include<sys/asmio.h>
#include<sys/defs.h>

  void outl(uint16_t port, uint32_t val){
  __asm__ __volatile__ ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

  uint32_t inl(uint16_t port)
{
    uint32_t ret;
   __asm__ __volatile__ ( "inl %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}



  void outb(uint16_t port, uint8_t val)
{
    __asm__ __volatile__ ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}


  uint8_t inb(uint16_t port)
{
    uint8_t ret;
   __asm__ __volatile__ ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}



void  enableinterupts(void) {
    __asm__  __volatile__ ("sti");

}

 void disableinterrupts(void){
    __asm__ __volatile__("cli");
}


void haltTillInterrupt(void){
   __asm__ __volatile__("cli;hlt");
}


  void io_wait(void)
{
  __asm__ __volatile__ ( "outb %%al, $0x80" : : "a"(0) );

}

