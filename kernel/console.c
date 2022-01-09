// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#include "console.h"

static void consputc(int);

static int panicked = 0;

static struct {
	struct spinlock lock;
	int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
	static char digits[] = "0123456789abcdef";
	char buf[16];
	int i;
	uint x;

	if(sign && (sign = xx < 0))
		x = -xx;
	else
		x = xx;

	i = 0;
	do{
		buf[i++] = digits[x % base];
	}while((x /= base) != 0);

	if(sign)
		buf[i++] = '-';

	while(--i >= 0)
		consputc(buf[i]);
}

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
	int i, c, locking;
	uint *argp;
	char *s;

	locking = cons.locking;
	if(locking)
		acquire(&cons.lock);

	if (fmt == 0)
		panic("null fmt");

	argp = (uint*)(void*)(&fmt + 1);
	for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
		if(c != '%'){
			consputc(c);
			continue;
		}
		c = fmt[++i] & 0xff;
		if(c == 0)
			break;
		switch(c){
		case 'd':
			printint(*argp++, 10, 1);
			break;
		case 'x':
		case 'p':
			printint(*argp++, 16, 0);
			break;
		case 's':
			if((s = (char*)*argp++) == 0)
				s = "(null)";
			for(; *s; s++)
				consputc(*s);
			break;
		case '%':
			consputc('%');
			break;
		default:
			// Print unknown % sequence to draw attention.
			consputc('%');
			consputc(c);
			break;
		}
	}

	if(locking)
		release(&cons.lock);
}

void
panic(char *s)
{
	int i;
	uint pcs[10];

	cli();
	cons.locking = 0;
	// use lapiccpunum so that we can call panic from mycpu()
	cprintf("lapicid %d: panic: ", lapicid());
	cprintf(s);
	cprintf("\n");
	getcallerpcs(&s, pcs);
	for(i=0; i<10; i++)
		cprintf(" %p", pcs[i]);
	panicked = 1; // freeze other CPU
	for(;;)
		;
}







#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory



int green = 0;
int kRow = 2;
int kCollumn = 22;
int cursorPos = 80*2-22;
int activeCollumn = 1;
int col = 0;
int tableActive = 0;

void removeOldPaint(){
	for(int i=0;i<10;i++){
	   crt[cursorPos+i] &= 0x00ff;
	   crt[cursorPos+i] |= 0x0F00;
	}
}	

void paintSelectedField(){	
	for(int i=0;i<10;i++){
	   crt[cursorPos+i] &= 0x00ff;
	   crt[cursorPos+i] |= 0xF000;
	}
}


int colorsBasic[10][2] = {
{0x0000, 0x0000},
{0x0100, 0x1000},
{0x0200, 0x2000},
{0x0300, 0x3000},
{0x0400, 0x4000},
{0x0500, 0x5000},
{0x0600, 0x6000},
{0x0700, 0x7000},
};

int colorsLight[10][2] = {
{0x0800, 0x8000},
{0x0900, 0x9000},
{0x0A00, 0xA000},
{0x0B00, 0xB000},
{0x0C00, 0xC000},
{0x0D00, 0xD000},
{0x0E00, 0xE000},
{0x0F00, 0xF000},
};



char oldText[10][23];

char table[10][23] = {"/---<FG>--- ---<BG>---\\",
			     "|Black     |Black     |",
			     "|Blue      |Blue      |",
			     "|Green     |Green     |",
			     "|Aqua      |Aqua      |",
			     "|Red       |Red       |",
		             "|Purple    |Purple    |",
			     "|Yellow    |Yellow    |",
		             "|White     |White     |",
			     "\\---------------------/"};


void saveText(){

	int posToSave = 80-23;
	int k = 2;
	
	for(int i=0; i<10;i++){
	   for(int j=0; j<23;j++){
	       oldText[i][j] = crt[posToSave];
	       posToSave = posToSave+1;
	   }	
	   posToSave = (80*k)-23;
	   k=k+1;
	}

}

void printOldText(){

	int printPos = 80-23;
	int k = 2;
	int mask = crt[0] & 0xff00;
	
	for(int i=0; i<10;i++){
	   for(int j=0; j<23;j++){
		
	       crt[printPos] = oldText[i][j] | mask; // STAVLJA TRENUTNU BOJU
	       printPos = printPos+1;
	   }	
	   printPos = (80*k)-23;
	   k=k+1;
	}

}

void changeTextColorBasic(){

	int printPos = 0;
	int k = 0;
	
	if(activeCollumn == 1){
		//menjam boju slova - DRUGI BAJT
	  for(int i=0; i<10; i++){
	   for(int j=0; j<57;j++){
	       crt[printPos+j] &= 0xf0ff;
	       crt[printPos+j] |= colorsBasic[kRow-2][activeCollumn-1];
	   }
	   k = k+1;
	   printPos = 80*k; 	
	  }
	  
	  for(int i=0; i<15; i++){
	   for(int j=0; j<81;j++){
	       crt[printPos+j] &= 0xf0ff;
	       crt[printPos+j] |= colorsBasic[kRow-2][activeCollumn-1];
	   }
	   k = k+1;
	   printPos = 80*k; 	
	  }
	 }
	if(activeCollumn == 2){
		//menjam boju pozadine - PRVI BAJT
	  for(int i=0; i<10; i++){
	   for(int j=0; j<57;j++){
	       crt[printPos+j] &= 0x0fff;
	       crt[printPos+j] |= colorsBasic[kRow-2][activeCollumn-1];
	   }
	   k = k+1;
	   printPos = 80*k; 	
	  }
	   for(int i=0; i<15; i++){
	    for(int j=0; j<81;j++){
	       crt[printPos+j] &= 0x0fff;
	       crt[printPos+j] |= colorsBasic[kRow-2][activeCollumn-1];
	   }
	   k = k+1;
	   printPos = 80*k; 	
	  }
	 }

}


void changeTextColorLight(){

	int printPos = 0;
	int k = 0;
	
	if(activeCollumn == 1){
		//menjam boju slova - DRUGI BAJT
	  for(int i=0; i<10; i++){
	   for(int j=0; j<57;j++){
	       crt[printPos+j] &= 0xf0ff;
	       crt[printPos+j] |= colorsLight[kRow-2][activeCollumn-1];
	   }
	   k = k+1;
	   printPos = 80*k; 	
	  }
	  
	  for(int i=0; i<15; i++){
	   for(int j=0; j<81;j++){
	       crt[printPos+j] &= 0xf0ff;
	       crt[printPos+j] |= colorsLight[kRow-2][activeCollumn-1];
	   }
	   k = k+1;
	   printPos = 80*k; 	
	  }
	 }
	if(activeCollumn == 2){
		//menjam boju pozadine - PRVI BAJT
	  for(int i=0; i<10; i++){
	   for(int j=0; j<57;j++){
	       crt[printPos+j] &= 0x0fff;
	       crt[printPos+j] |= colorsLight[kRow-2][activeCollumn-1];
	   }
	   k = k+1;
	   printPos = 80*k; 	
	  }
	  for(int i=0; i<15; i++){
	   for(int j=0; j<81;j++){
	       crt[printPos+j] &= 0x0fff;
	       crt[printPos+j] |= colorsLight[kRow-2][activeCollumn-1];
	   }
	   k = k+1;
	   printPos = 80*k; 	
	  }
	 }

}



void createTable(){

	int printPos = 80-23;
	int k = 2;
	
	for(int i=0; i<10;i++){
	   for(int j=0; j<23;j++){
	       crt[printPos] = table[i][j] | 0x0F00;
	       printPos = printPos+1;
	   }	
	   printPos = (80*k)-23;
	   k=k+1;
	}
	
	int paintPos = 80*2-22;
	for(int i=0;i<10;i++){
	   crt[cursorPos+i] &= 0x00ff;
	   crt[cursorPos+i] |= 0xF000;
	}
	
}



// 获取光标位置
int
getcurpos(void)
{
  int pos;
  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);
  return pos;
}

// 设置光标位置
void
setcurpos(int pos)
{
  // 范围检查
  if(pos < 0)
    pos = 0;
  else if(pos >= MAX_CHAR)
    pos = MAX_CHAR - 1;

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
}

// 在屏幕的 pos 位置处设置字符（不移动光标），传入的 int c 仅低 16 位有效
// 其中从右往左数：
// 0~7   字符的 ascii 码值
// 8~11  文本色
// 12~15 背景色
void
showc(int pos, int c)
{
  crt[pos] = c;
}

static void
cgaputc(int c)
{
  int pos = getcurpos();

  // 回车键 加上这一行剩余的空白数，即 pos 移到下一行行首
  if(c == '\n')
    pos += SCREEN_WIDTH - pos%SCREEN_WIDTH;
  else if(c == BACKSPACE){ // 退格键 pos 退 1 个
    if(pos > 0) --pos;
    crt[pos] = ' ';
  } else showc(pos++, (c&0xff) | 0x0700);

  if(pos < 0 || pos > MAX_CHAR)
    panic("pos under/overflow");

  if((pos/SCREEN_WIDTH) >= (SCREEN_HEIGHT-1)){  // Scroll up.
    memmove(crt, crt+SCREEN_WIDTH, sizeof(crt[0])*(SCREEN_HEIGHT-2)*SCREEN_WIDTH);
    pos -= SCREEN_WIDTH;
    memset(crt+pos, 0, sizeof(crt[0])*((SCREEN_HEIGHT-1)*SCREEN_WIDTH - pos));
  }

  setcurpos(pos);
  showc(pos, (crt[pos]&0xff) | 0x0700);
}


// static void
// cgaputc(int c)
// {


// 	int pos;

// 	// Cursor position: col + 80*row.
// 	outb(CRTPORT, 14);
// 	pos = inb(CRTPORT+1) << 8;
// 	outb(CRTPORT, 15);
// 	pos |= inb(CRTPORT+1);


	
// 	if(c == '\n')
// 		pos += 80 - pos%80;
// 	else if(c == BACKSPACE){
// 		if(pos > 0) --pos;

// 	}
// 	else{
// 		int mask = crt[0] & 0xff00;
// 		crt[pos++] = (c&0xff) | mask;  
// 	}

// 	if(pos < 0 || pos > 25*80)
// 		panic("pos under/overflow");

// 	if((pos/80) >= 24){  // Scroll up.
// 		memmove(crt, crt+80, sizeof(crt[0])*23*80);
// 		pos -= 80;
// 		memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
// 		int mask = crt[0] & 0xff00;
		
// 		for(int i=0; i<80*25; i++)
// 		   crt[i] |= mask;
			
// 	}
	
// 	int mask = crt[0] & 0xff00;
// 	outb(CRTPORT, 14);
// 	outb(CRTPORT+1, pos>>8);
// 	outb(CRTPORT, 15);
// 	outb(CRTPORT+1, pos);
// 	crt[pos] = ' ' | mask;
// }

void
consputc(int c)
{
	if(panicked){
		cli();
		for(;;)
			;
	}

	if(c == BACKSPACE){
		uartputc('\b'); uartputc(' '); uartputc('\b');
	} else
		uartputc(c);
	cgaputc(c);
}

#define INPUT_BUF 128
struct {
	char buf[INPUT_BUF];
	uint r;  // Read index
	uint w;  // Write index
	uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x
#define A(x) ((x)-'@') // ALT-x

static int showflag = 1;      // 为 0 时不打印到屏幕上
static int bufflag = 1;       // 为 0 时不缓存键盘输入，马上读取 1 个字符
static int backspaceflag = 1; // 为 0 时不拦截退格键

// 设置 showflag, bufflag, backspaceflag
void
setflag(int sf, int bf, int bsf)
{
  showflag = sf;
  bufflag = bf;
  backspaceflag = bsf;
}


void
consoleintr(int (*getc)(void))
{
	int c, doprocdump = 0;

	acquire(&cons.lock);
	while((c = getc()) >= 0){
	     if(tableActive == 1){
		if(c == 'w'){
			//gore
			removeOldPaint();
			kRow = kRow-1;
			if (kRow < 2){kRow = 9;}
			cursorPos = 80*kRow-kCollumn;
			paintSelectedField();
		

		}else if(c == 's'){
			//dole
			removeOldPaint();
			kRow = kRow+1;
			if (kRow > 9){kRow = 2;}
			cursorPos = 80*kRow-kCollumn;
			paintSelectedField();
		
		
		
		}else if(c == 'a'){
			//levo
			removeOldPaint();
			if(activeCollumn == 1){
		   		activeCollumn = 2;
		  		kCollumn = 11;
			}
			else{
		   		activeCollumn = 1;
		   		kCollumn = 22;
			}
		
			cursorPos = 80*kRow-kCollumn;
			paintSelectedField();
		
		}else if(c == 'd'){
			//desno
			removeOldPaint();
			if(activeCollumn == 1){
		   		activeCollumn = 2;
		   		kCollumn = 11;
			}else{
		   		activeCollumn = 1;
		  		 kCollumn = 22;
			}
		
			cursorPos = 80*kRow-kCollumn;
			paintSelectedField();
		
		}else if(c == 'e'){
			//osnovni oblik boje
			changeTextColorBasic();

		}else if(c == 'r'){
			//svetliji oblik boje
			changeTextColorLight();
		}}
		
		switch(c){
		//case A('C'): //AltC is pressed
		//	if(col == 0)
		//	   col=1;
		//	break;
		//case A('O'): //AltO is pressed
		//	if(col == 1)
		//	   col=2;
		//	break;
		//case A('L'): //AltL is pressed
		//	if(col == 2){
		//	   col=0;
		//		if(tableActive == 0){
		//			tableActive = 1;
					// CREATE TABLE HERE <------
		//			saveText();
		//			createTable();


		//		}else{
		//			tableActive = 0;
					// REMOVE TABLE HERE <------
		//			printOldText();
		//			cursorPos = 80*2-22;	
 		//			kRow = 2;
 		//			kCollumn = 22;
		//			activeCollumn = 1;
		//		}
		//	}
		//	break;
		
		case C('P'):  // Process listing.
			// procdump() locks cons.lock indirectly; invoke later
			doprocdump = 1;
			break;
		case C('U'):  // Kill line.
			while(input.e != input.w &&
			      input.buf[(input.e-1) % INPUT_BUF] != '\n'){
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		case C('H'): case '\x7f':  // Backspace
		    if (backspaceflag){
			    if(input.e != input.w){
				    input.e--;
				    consputc(BACKSPACE);
			    }
			    break;
			}
		default:
			if(c == 'a'-82){
		            if(tableActive == 0){
				tableActive = 1;
					// CREATE TABLE HERE <------
				saveText();
				createTable();
				}
			    else{
				tableActive = 0;
					// REMOVE TABLE HERE <------
				printOldText();
				cursorPos = 80*2-22;	
 				kRow = 2;
 				kCollumn = 22;
				activeCollumn = 1;
				}
			    c = 'a'-97;
			}
			if(tableActive == 0)
			if(c != 0 && input.e-input.r < INPUT_BUF){
				c = (c == '\r') ? '\n' : c;
				input.buf[input.e++ % INPUT_BUF] = c;
				if (showflag) consputc(c);
				
				if(!bufflag ||c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
					input.w = input.e;
					wakeup(&input.r);
						
				}
			
			}
			break;
                      
		}
	   
		
	}
	release(&cons.lock);
	if(doprocdump) {
		procdump();  // now call procdump() wo. cons.lock held
	}
}

int
consoleread(struct inode *ip, char *dst, int n)
{
	uint target;
	int c;

	iunlock(ip);
	target = n;
	acquire(&cons.lock);
	while(n > 0){
		while(input.r == input.w){
			if(myproc()->killed){
				release(&cons.lock);
				ilock(ip);
				return -1;
			}
			sleep(&input.r, &cons.lock);
		}
		c = input.buf[input.r++ % INPUT_BUF];
		if(c == C('D')){  // EOF
			if(n < target){
				// Save ^D for next time, to make sure
				// caller gets a 0-byte result.
				input.r--;
			}
			break;
		}
		*dst++ = c;
		--n;
		if(c == '\n')
			break;
	}
	release(&cons.lock);
	ilock(ip);

	return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
	int i;

	iunlock(ip);
	acquire(&cons.lock);
	for(i = 0; i < n; i++)
		consputc(buf[i] & 0xff);
	release(&cons.lock);
	ilock(ip);

	return n;
}

void
consoleinit(void)
{
	initlock(&cons.lock, "console");

	devsw[CONSOLE].write = consolewrite;
	devsw[CONSOLE].read = consoleread;
	cons.locking = 1;

	ioapicenable(IRQ_KBD, 0);
}

// 清屏
void
clearscreen(void)
{
  memset(crt, 0, sizeof(crt[0])*MAX_CHAR);
  setcurpos(0);
}

// 备份当前屏幕上的所有字符
void
backupscreen(ushort *backup, int nbytes)
{
  memmove(backup, crt, nbytes);
}

// 恢复屏幕内容
void
recoverscreen(ushort *backup, int nbytes)
{
  int pos = nbytes / sizeof(crt[0]); // crt中的字符为ushort类型，占2字节
  clearscreen();
  memmove(crt, backup, nbytes);
  setcurpos(pos);
}