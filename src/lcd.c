//#define DEBUG

#include "stc15.h"
#include "utils.h"
#include "lcd.h"


#define LCDINC 2
#define LCDDEC 0
#define LCDSHIFT 1
#define LCDNOSHIFT 0
#define LCDCURSOR 2
#define LCDNOCURSOR 0
#define LCDBLINK 1
#define LCDNOBLINK 0
#define LCDSCROLL 8
#define LCDNOSCROLL 0
#define LCDLEFT 0
#define LCDRIGHT 4
#define LCD2LINE 8
#define LCD1LINE 0
#define LCD10DOT 4
#define LCD7DOT 0

#define CR 13 // \r
#define TAB 9 // \n
#define LF 10 // \r



static int row, col;
static int OpenFlag = 0;
static int ErrorTimeout = 0;
static int Error = 0;

#define CLEAR_BIT(port, bit) (port &= ~(_BV(bit)))
#define CLEAR_BITS(port, bits) (port &= ~(bits))
#define SET_BIT(port, bit) (port |= _BV(bit))

//#define DISABLE_INTERRUPTS() __critical{
//#define ENABLE_INTERRUPTS()  }
#define DISABLE_INTERRUPTS() {
#define ENABLE_INTERRUPTS()  }

#define LCD_E P3_5
#define LCD_RW P3_6
#define LCD_RS P3_7
#define LCD_BUSY P2_7 //LCD D7

static void outCsrBlindNibble(unsigned char command);
static void outCsrBlind(unsigned char command);
static void outCsr(unsigned char command);
static char readBusy(void);
static void wait_busy(void);
static void to_row(unsigned char row_to);
static void LCD_OutChar(unsigned char c);

static void outCsrBlindNibble(unsigned char command) {
//	CLEAR_BITS(PORTC, _BV(LCD_E) | _BV(LCD_RS) | _BV(LCD_RW));
	P3 &= ~(0x7 << 5); //clear LCD_E, LCD_RS, LCD_RW
	_delay_us(50);
	P2 = ((command & 0xf) << 4);
	//CLEAR_BIT(PORTC, LCD_E); //E = 0
	//CLEAR_BIT(PORTC, LCD_RS); //control
	//CLEAR_BIT(PORTC, LCD_RW); //write
	_delay_us(100);
	LCD_E = 1;
	_delay_us(100);
	LCD_E = 0;
}

static void outCsrBlind(unsigned char command) {
	outCsrBlindNibble(command >> 4); // ms nibble, E=0, RS=0
	_delay_us(100);
	outCsrBlindNibble(command); // ls nibble, E=0, RS=0
	_delay_us(100); // blind cycle 90 us wait
}

static void outCsr(unsigned char command) {
	DISABLE_INTERRUPTS();
	outCsrBlindNibble(command >> 4); // ms nibble, E=0, RS=0
	outCsrBlindNibble(command); // ls nibble, E=0, RS=0
	ENABLE_INTERRUPTS();
	wait_busy();
}

#define SET_BUSY_IN()  do {P2M1 |= (1<<7);  P2M0 &= ~(1<<7);} while(0)
#define SET_BUSY_OUT() do {P2M1 &= ~(1<<7); P2M0 |= (1<<7); } while(0)

//returns 1 if busy, 0 otherwise
static char readBusy() {
	unsigned char oldP2 = P2;
	__bit busy;

	LCD_RS = 0; //control
	LCD_RW = 1; //read
	LCD_E = 1;
	SET_BUSY_IN();

	//wait
	_delay_us(100); // blind cycle 100us wait
	//read busy flag
	busy = LCD_BUSY;
	SET_BUSY_OUT();
	LCD_E = 0;
	LCD_E = 1;
	//wait
	_delay_us(100); // blind cycle 100us wait
	LCD_E = 0;
	P2 = oldP2;

	return busy;
}

//sets ErrorTimeout flag if busy signal not cleared in time
static void wait_busy() {
	unsigned int i;
	for (i = 0; i < 100; i++){
		if (!readBusy()){
			return;
		}
		_delay_ms(1);
	}

	ErrorTimeout = 1;
	return;
}

static void LCD_OutChar(unsigned char c) {
	unsigned char lower = (c & 0x0f);
	DISABLE_INTERRUPTS();
	wait_busy();
	//output upper 4 bits:
	LCD_E = 0;
	LCD_RS = 1; //data
	LCD_RW = 0; //write
	P2 = ((c & 0xf0));
	LCD_E = 1;
	_delay_us(100);
	LCD_E = 0;
	//output lower 4 bits:
	P2 = ((c & 0xf) << 4);
	LCD_E = 1;
	_delay_us(100);
	LCD_E = 0;
	ENABLE_INTERRUPTS();
	wait_busy();
}

void LCD_Open(void) {
	static int open_error = 0;
	if (!OpenFlag) {
		//set ports to push-pull output M[1:0] = b01
		//P2 entire port
		P2M1 = 0;
		P2M0 = 0xff;
		//P3 pins 7:4
		P3M1 &= ~(0xf0);
		P3M0 |= (0xf0);

		_delay_ms(30); // to allow LCD powerup
		outCsrBlindNibble(0x03); // (DL=1 8-bit mode)
		_delay_ms(5); //  blind cycle 5ms wait
		outCsrBlindNibble(0x03); // (DL=1 8-bit mode)
		_delay_us(100); // blind cycle 100us wait
		outCsrBlindNibble(0x03); // (DL=1 8-bit mode)
		_delay_us(100); //  blind cycle 100us wait (not called for, but do it anyway)
		outCsrBlindNibble(0x02); // DL=1 switch to 4 bit mode (only high 4 bits are sent)
		_delay_us(100); // blind cycle 100 us wait
		//set increment, no shift
		outCsrBlind(0x4 + LCDINC + LCDNOSHIFT);
#ifdef DEBUG
		//set display on, cursor on, blink on:
		outCsrBlind(0x0c + LCDCURSOR + LCDBLINK);
#else
		//set display on, cursor and blink off:
		outCsrBlind(0x0c + LCDNOCURSOR + LCDNOBLINK);
#endif
		//set display shift on and to the right
		outCsrBlind(0x10 + LCDNOSCROLL + LCDRIGHT);
		//set 4-bit mode, 2 line, 5x7 display:
		outCsrBlind(0x20 + LCD2LINE + LCD7DOT);
		//clear display
		LCD_Clear();
		_delay_ms(50);
		if (!readBusy()) {
			OpenFlag = 1;
		} else {
			open_error = 1;
		}
	} else { //display already open
		open_error = 1;
	}
}

//row and columns indexed from 0
void LCD_GoTo(unsigned int row_to, unsigned int col_to) {
	if (row_to < MAX_ROWS && col_to < MAX_CHARS_PER_LINE) {
		outCsr(0x80 + 0x40 * row_to + col_to); //set ddram address to position
		row = row_to;
		col = col_to;
	} else {
		Error = 1;
	}
}

static void to_row(unsigned char row_to) {
	if (row_to == 0) {
		outCsr(0x80);//set address to start of row 0
		row = 0;
	} else {
		outCsr(0xc0);// set address to row 1
		row = 1;
	}
	col = 0;
}

//row indexed from 0
void LCD_SingleLineGoTo(unsigned int spot_to) {
	LCD_GoTo(spot_to / MAX_CHARS_PER_LINE, spot_to % MAX_CHARS_PER_LINE);
}

void LCD_OutString(const char *string) {
	const char *s;
	for (s = string; *s; s++) {
		TERMIO_PutChar(*s);
	}
}

short TERMIO_PutChar(unsigned char letter) {
	if (letter == CR || letter == '\n') {
		LCD_Clear();
	} else if (letter == TAB || letter == '\t') {
		if (row == 0) {
			to_row(1);
		} else {
			to_row(0);
		}
	} else {
		if (col > MAX_CHARS_PER_LINE) {
			if (row == 0) {
				to_row(1);
			} else {
				to_row(0);
			}
		}
		col++;
		LCD_OutChar(letter);
	}

	return 1;
}

void LCD_OutNibble(uint8_t x){
	x &= 0xf; //ensure only bottom nibble
	if (x <= 9){
		TERMIO_PutChar(x + '0');
	} else {
		TERMIO_PutChar(x - 10 + 'a');
	}
}

void LCD_Clear() {
	if (OpenFlag) {
		outCsr(0x01);
	} else {
		outCsrBlind(0x01);
	}
	row = 0;
	col = 0;
}

unsigned char LCD_Timeout_Error(void) {
	if (ErrorTimeout != 0) {
		return 1;
	}
	return 0;
}

