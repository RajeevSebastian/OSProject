//#include<sys/timer.h>
#include<sys/kprintf.h>
#include<sys/pic.h>
#include<sys/defs.h>


static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ __volatile__ ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}


 static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
   __asm__ __volatile__ ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}





void timer_handler(){
  static int interruptcnt = 0;
  interruptcnt++;
// kprintf("interrupt count %d \n",interruptcnt );
  static long timesinceboot = 0;
  if(interruptcnt == 18){
     timesinceboot++;
     printTimeSinceBoot(timesinceboot);
    // kprintf("time since boot %d\n",timesinceboot);
     interruptcnt = 0;   	 
 }
//timer connected at pin 0 of the master PIC 
  PIC_sendEOI(0);
}





