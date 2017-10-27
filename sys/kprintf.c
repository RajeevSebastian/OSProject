#include <sys/kprintf.h>
#include<sys/stdarg.h>
#include<sys/defs.h>
#define VIDEOSTART (char *)0xB8000
#define VIDEOEND (char *)0xB8000 + 160*23


void  writeToScreen( char * str,int* charWritten){

volatile char* start = VIDEOSTART + *charWritten*2;
int strleng =0;

	while(*str){
		
		while(start != VIDEOEND && *str){
			*start = *str;
			start+=2;
			strleng++;
			str++;
		}
		if(start == VIDEOEND){
			start = VIDEOSTART;
			*charWritten = 0;
		}
		
	}

*charWritten = *charWritten + strleng;

}

void newline(int* charWritten){


 int numberOfWhitespaces = 80 - (*charWritten % 80);

 *charWritten += numberOfWhitespaces;

}

void carriageReturn(int * charWritten){

	int numberOfCharsCurrentLine = *charWritten % 80;
	*charWritten -= numberOfCharsCurrentLine;
}


void itoa(unsigned long value, char * str,int base){
	char *ptr =str;
	char charmap[] = "0123456789abcdef";
	char temp; 
	if(base == 10 && value < 0){
	   *ptr++ = '-';
	    value = -1 * value;
	}else if(value == 0){
		str[0] ='0';
		str[1] = '\0';
	}else{
        char * low = ptr;
	while(value){
		*ptr++ = charmap[value%base];
		value =value  /base;
	} 
	*ptr='\0';
	ptr --;
	while(low < ptr){
		temp = *low;
		*low = *ptr;
		*ptr = temp;
		low++;
		ptr--; 
	}

	}
	
}


void printTimeSinceBoot(long value){
   char time[100];
   itoa(value,time,10);
   char *s  =time;
   int len=0;
   while(*s != '\0'){
     len++;
     s++;
   }
   char * timerposition =  VIDEOEND + (80 - len) *2;
   s = time;
  while(*s != '\0'){
     *timerposition = *s;
      timerposition +=2;
      s++;   
  }

}


void printkey(uint8_t value){
 char key[10];
 itoa(value,key,16);
  char *s  =key;
   int len=0;
   while(*s != '\0'){
     len++;
     s++;
   }
   char * keyposition =  VIDEOEND + (75 - len) *2;
   s = key;
  while(*s != '\0'){
     *keyposition = *s;
      keyposition +=2;
      s++;
  }



}

void printglyph(uint8_t ascii,int shiftPressed , int controlPressed){
  char glyphwcontrol[7] ="    ^";
  char glyphwocontrol[7] ="     ";
  glyphwcontrol[5] = ascii;
  glyphwcontrol[6] = '\0';
  glyphwocontrol[5] = ascii;
  glyphwocontrol[6] ='\0'; 
  int len=sizeof(glyphwcontrol);
  char *s;
  if(controlPressed){
	s = glyphwcontrol;
  }else{
	s = glyphwocontrol;
  }
  char *glyphposition = VIDEOEND + (75-len)*2;
  while(*s != '\0'){
	*glyphposition =*s;
	glyphposition += 2;
	s++;

  }


}



void kprintf(const char *fmt, ...)
{
  va_list argptr;
  char *p;
  char *str;
  char c[2];
  c[1] = '\0';
  int intvalue;
  long hexvalue;	
  unsigned long addressvalue;
  char buffer[1000];
  static int numOfCharactersWritten =0;
  
  //writeToScreen("Rajeev",&numOfCharactersWritten);		  
  //newline(&numOfCharactersWritten);
  p = (char *)fmt;
  va_start(argptr,fmt);
  while(*p){

	switch(*p){
	
	case '%':
	   p++;
	   switch(*p){
	     case 'c':
		c[0] = va_arg(argptr,int);
		writeToScreen(c,&numOfCharactersWritten);
	     break;
	     case 'd':
		intvalue  = va_arg(argptr,int);
		itoa(intvalue,buffer,10);
		writeToScreen(buffer,&numOfCharactersWritten);
	     break;
	     case 'x':
		hexvalue = va_arg(argptr,long);
		itoa(hexvalue,buffer,16);
		writeToScreen(buffer,&numOfCharactersWritten); 
	     break;
             case 's':
		str =va_arg(argptr,char*);
		writeToScreen(str,&numOfCharactersWritten);	
 	     break;
	     case 'p':
		addressvalue = (uint64_t)va_arg(argptr,void *);
		buffer[0] = '0';
		buffer[1] ='x';
		itoa(addressvalue,&buffer[2],16);
		writeToScreen(buffer,&numOfCharactersWritten);
		//print to screen
	     break;	
		
	   }
	
	break;
	
	case '\n':
	    newline(&numOfCharactersWritten);
	break;
		
	case '\r':
	   carriageReturn(&numOfCharactersWritten);	
	break;
	
	default:
	      	//printtoscreen
		c[0] = *p;
		writeToScreen(c,&numOfCharactersWritten);
	  break;
	}
	p++;

  }
  

   va_end(argptr);
}


