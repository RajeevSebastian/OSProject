#include<sys/defs.h>
#include<sys/pic.h>
#include<sys/asmio.h>

#define PIC1		0x20		
#define PIC2		0xA0		
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)


#define ICW1_ICW4	0x01		
#define ICW1_SINGLE	0x02		
#define ICW1_INTERVAL4	0x04		
#define ICW1_LEVEL	0x08		
#define ICW1_INIT	0x10		
 
#define ICW4_8086	0x01		
#define ICW4_AUTO	0x02		
#define ICW4_BUF_SLAVE	0x08		
#define ICW4_BUF_MASTER	0x0C		
#define ICW4_SFNM	0x10		



#define PIC_READ_IRR    0x0a    
#define PIC_READ_ISR    0x0b    
#define PIC_EOI		0x20



void PIC_sendEOI(unsigned char irq)
{
	if(irq >= 8)
		outb(PIC2_COMMAND,PIC_EOI);
 
	outb(PIC1_COMMAND,PIC_EOI);
}
 

static uint16_t __pic_get_irq_reg(int ocw3)
{
    
    
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}
 

uint16_t pic_get_irr(void)
{
    return __pic_get_irq_reg(PIC_READ_IRR);
}
 

uint16_t pic_get_isr(void)
{
    return __pic_get_irq_reg(PIC_READ_ISR);
}


void PIC_remap(int offset1, int offset2)
{	
 
	outb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4);  
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 
	io_wait();
	outb(PIC2_DATA, offset2);                 
	io_wait();
	outb(PIC1_DATA, 4);                       
	io_wait();
	outb(PIC2_DATA, 2);                       
	io_wait();
 
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
 
	outb(PIC1_DATA, 0xFC);   
	outb(PIC2_DATA, 0xFF);
}
