#include "types.h"
#include "x86.h"
#include "defs.h"
#include "kbd.h"

static int cola = 0;


int
kbdgetc(void)
{
	static uint shift;
	static uchar *charcode[7] = {
		normalmap, shiftmap, ctlmap, ctlmap, altmap, altmap, altmap
	};
	uint st, data, c;

	st = inb(KBSTATP);
	if((st & KBS_DIB) == 0)
		return -1;
	data = inb(KBDATAP);

	if(data == 0xE0){
		shift |= E0ESC;
		return 0;
	} else if(data & 0x80){
		// Key released
		data = (shift & E0ESC ? data : data & 0x7F);
		shift &= ~(shiftcode[data] | E0ESC);
		return 0;
	} else if(shift & E0ESC){
		// Last character was an E0 escape; or with 0x80
		data |= 0x80;
		shift &= ~E0ESC;
	}

	shift |= shiftcode[data];
	shift ^= togglecode[data];
	c = charcode[shift & (CTL | SHIFT)][data];
	if(shift & CAPSLOCK){
		if('a' <= c && c <= 'z')
			c += 'A' - 'a';
		else if('A' <= c && c <= 'Z')
			c += 'a' - 'A';
	}


	if(shift & ALT){
	   if(c=='c'){
		cola = 1;
		c = 'a'-97;
	   }
	   else if(c=='o'){
		if(cola == 1){
			cola=2;
			c = 'a'-97;
		}else{  
			cola = 0;
			c = 'a'-97;
			}		
		}
	   else if(c=='l'){
		if(cola == 2){
			cola=0;
			c = 'a'-82;
		}else{  
			cola=0;
			c = 'a'-97;
			}
	   }else{
		cola = 0;
		c = 'a'-97;
		}


	}
		

	return c;
}

void
kbdintr(void)
{
	consoleintr(kbdgetc);
}
