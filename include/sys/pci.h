#ifndef _PCI_H
#define PCI_H

uint32_t pciReadRegister(uint32_t bus , uint32_t device , uint32_t func , uint32_t offset);
void checkAllBuses();


#endif
