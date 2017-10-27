#include<sys/defs.h>
#ifndef __KPRINTF_H
#define __KPRINTF_H

void kprintf(const char *fmt, ...);
void printTimeSinceBoot(long value);
void printkey(uint8_t value);
void printglyph(uint8_t asciicode,int shiftpressed , int controlPressed);
#endif
