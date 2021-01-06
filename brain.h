#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// from lcd-hello.c
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#define MAX_GUESS 12

enum Color {red,green,blue};
static const char Color_String[] = {'R', 'G', 'B'};

// from lcd-hello.c
#ifndef TRUE
#  define TRUE  (1==1)
#  define FALSE (1==2)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)
#define	INPUT			 0
#define	OUTPUT			 1
#define	LOW			 0
#define	HIGH			 1
#define STRB_PIN 24
#define RS_PIN   25
#define DATA0_PIN 23
// #define DATA1_PIN 17
#define DATA1_PIN 10
#define DATA2_PIN 27
#define DATA3_PIN 22
#define	LCD_CLEAR	0x01
#define	LCD_HOME	0x02
#define	LCD_ENTRY	0x04
#define	LCD_CTRL	0x08
#define	LCD_CDSHIFT	0x10
#define	LCD_FUNC	0x20
#define	LCD_CGRAM	0x40
#define	LCD_DGRAM	0x80
// Bits in the entry register
#define	LCD_ENTRY_SH		0x01
#define	LCD_ENTRY_ID		0x02
// Bits in the control register
#define	LCD_BLINK_CTRL		0x01
#define	LCD_CURSOR_CTRL		0x02
#define	LCD_DISPLAY_CTRL	0x04
// Bits in the function register
#define	LCD_FUNC_F	0x04
#define	LCD_FUNC_N	0x08
#define	LCD_FUNC_DL	0x10
#define	LCD_CDSHIFT_RL	0x04
// Mask for the bottom 64 pins which belong to the Raspberry Pi
//	The others are available for the other devices
#define	PI_GPIO_MASK	(0xFFFFFFC0)

#define WAIT_FOR_ENTER printf("=== Press ENTER to continue ===");fgetc();

struct lcdDataStruct {
  int bits, rows, cols;
  int rsPin, strbPin;
  int dataPins [8];
  int cx, cy;
};

//static int lcdControl;
//static volatile unsigned int gpiobase;
//static volatile uint32_t *gpio;
/*
// char data for the CGRAM, i.e. defining new characters for the display
static unsigned char newChar [8] = {
  0b11111,
  0b10001,
  0b10001,
  0b10101,
  0b11111,
  0b10001,
  0b10001,
  0b11111,
};
// bit pattern to feed into lcdCharDef to define a new character
static unsigned char hawoNewChar [8] = {
  0b11111,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b11111,
};*/

// My functionis
void initGPIO();
void initLCD();
void pinWrite(volatile uint32_t *gpio, int pinACT, int theValue);
void strobe (volatile uint32_t *gpio, const struct lcdDataStruct *lcd);
void sendDataCmd (volatile uint32_t *gpio, const struct lcdDataStruct *lcd,
    unsigned char data);
void lcdPutCommand (volatile uint32_t *gpio, const struct lcdDataStruct *lcd,
    unsigned char command);
void lcdPut4Command (volatile uint32_t *gpio, const struct lcdDataStruct *lcd,
    unsigned char command);
void lcdHome (volatile uint32_t *gpio, struct lcdDataStruct *lcd);
void lcdPosition(volatile uint32_t *gpio ,struct lcdDataStruct *lcd, int x, int y);
void lcdDisplay(volatile uint32_t *gpio ,struct lcdDataStruct *lcd, int state);
void lcdCursor(volatile uint32_t *gpio, struct lcdDataStruct *lcd, int state);
void lcdCursorBlink(volatile uint32_t *gpio, struct lcdDataStruct *lcd, int state);
void lcdPutchar(volatile uint32_t *gpio, struct lcdDataStruct *lcd, unsigned char data);
void lcdPuts(volatile uint32_t *gpio, struct lcdDataStruct *lcd, const char *string);
void lcdClear (volatile uint32_t *gpio, struct lcdDataStruct *lcd);

