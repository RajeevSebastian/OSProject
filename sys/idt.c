#include<sys/defs.h>
#include<sys/idt.h>
#include<sys/kprintf.h>


extern void isr32();
extern void isr33();
struct idt_descr 
{
  uint16_t offset_low;  //lower 16 bits of the isr handler
  uint16_t selector;    //code segement selector
  uint8_t zero;         //set to zero
  uint8_t type_attr;    //type and attributes
  uint16_t offset_mid;  //middle 16 bits of the isr address
  uint32_t offset_high; //higher 32 bits og the isr address
  uint32_t alsozero;        //set to zero
} __attribute__ ((packed));


struct idt_ptr
{
 uint16_t size;
 uint64_t base;
} __attribute__((packed));

struct idt_descr idt_table [256] = {};
struct idt_ptr idt_table_ptr;


void load_idt(struct idt_ptr * idt_ptr){
 long location=0;
     __asm__ __volatile__(
        "lidt %0 \n\t"
        "sidt %1 \n\t"
         :
         :"m"(*idt_ptr),"m"(location)
         :
     );
     kprintf("Address from SIDT %p\n",location);


}

void _x86_64_asm_lidt(struct idt_ptr *idt_ptr);

void idt_set_gate(uint8_t num, uint64_t base,uint16_t selector,uint8_t flags,uint8_t zero){
   idt_table[num].offset_low = (base & 0x0FFFF);
   idt_table[num].offset_mid = ((base>>16) & 0xFFFF);
   idt_table[num].offset_high =((base>>32) & 0xFFFFFFFF);
   idt_table[num].zero =zero;
   idt_table[num].type_attr =flags;
   idt_table[num].selector =selector;
}


void init_idt(){
     idt_table_ptr.base = (uint64_t)idt_table;
     idt_table_ptr.size = sizeof(idt_table);
//     kprintf("base address %p\n",idt_table_ptr.base);
//     kprintf("IDT size %d\n", idt_table_ptr.size);
//     kprintf("Address of idt_table_ptr %p \n",&idt_table_ptr);
     idt_set_gate(32,(uint64_t)&isr32,0x08,0x8E,0);	
     idt_set_gate(33,(uint64_t)&isr33,0x08,0x8E,0);  	
//     load_idt(&idt_table_ptr);
      _x86_64_asm_lidt (&idt_table_ptr);
	
}


