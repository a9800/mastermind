#include "brain.h"
static int lcdControl;

void pinWrite(volatile uint32_t *gpio, int pinACT, int theValue){
  int fSel = pinACT/10, shift;
  if(pinACT >= 10){
    shift = (pinACT%10) * 3;
  }else{
    shift = pinACT * 3;
  }

  *(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (1 << shift);

  int low = (pinACT < 32) ? 10 : 11;
  int high = (pinACT < 32) ? 7 : 8;
  if ((pinACT & 0xFFFFFFC0 ) == 0){
	  int off = (theValue == LOW) ? low : high;
    *(gpio + off) = 1 << (pinACT & 31);
  }else{
    fprintf(stderr, "only supporting on-board pins\n");
    exit(1);
  }
}

// TODO: nuke this
void delay(unsigned int howLong) {
  struct timespec sleeper, dummy;

  sleeper.tv_sec  = (time_t)(howLong / 1000);
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;
  nanosleep (&sleeper, &dummy);
}

// TODO: nuke this
void delayMicroseconds (unsigned int howLong) {
  struct timespec sleeper;
  unsigned int uSecs = howLong % 1000000;
  unsigned int wSecs = howLong / 1000000;

  if (howLong == 0)
    return;
//  else if (howLong < 100)
//    delayMicrosecondsHard (howLong);

  sleeper.tv_sec  = wSecs;
  sleeper.tv_nsec = (long)(uSecs * 1000L);
  nanosleep (&sleeper, NULL);
}

void strobe (volatile uint32_t *gpio, const struct lcdDataStruct *lcd) {
  pinWrite(gpio, lcd->strbPin, 1);
  delayMicroseconds(50);
  pinWrite(gpio, lcd->strbPin, 0);
  delayMicroseconds(50);
}

void sendDataCmd (volatile uint32_t *gpio, const struct lcdDataStruct *lcd, 
    unsigned char data) {
  register unsigned char myData = data;
  unsigned char i, d4;

  if (lcd->bits == 4)   {
    d4 = (myData >> 4) & 0x0F;
    for (i=0;i<4;++i) {
      pinWrite (gpio, lcd->dataPins [i], (d4 & 1));
      d4 >>= 1 ;
    }
    strobe (gpio, lcd) ;
    d4 = myData & 0x0F;

    for (i=0;i<4;++i) {
      pinWrite (gpio, lcd->dataPins [i], (d4 & 1));
      d4 >>= 1 ;
    }
  } else {
    for (i=0;i<8;++i) {
      pinWrite (gpio, lcd->dataPins [i], (myData & 1));
      myData >>= 1;
    }
  }
  strobe (gpio, lcd);
}

void lcdPutCommand (volatile uint32_t *gpio, const struct lcdDataStruct *lcd,
    unsigned char command) {
#ifdef DEBUG
  fprintf(stderr, "lcdPutCommand: digitalWrite(%d,%d) and sendDataCmd(%d,%d)\n", lcd->rsPin,   0, lcd, command);
#endif
  pinWrite(gpio, lcd->rsPin, 0);
  sendDataCmd(gpio, lcd, command);
  delay(2);
}

void lcdPut4Command (volatile uint32_t *gpio, const struct lcdDataStruct *lcd,
    unsigned char command) {
  register unsigned char myCommand = command;
  register unsigned char i;

  pinWrite(gpio, lcd->rsPin, 0);

  for (i=0;i<4;++i) {
    pinWrite(gpio, lcd->dataPins [i], (myCommand & 1));
    myCommand >>= 1;
  }
  strobe(gpio, lcd);
}

void lcdHome (volatile uint32_t *gpio, struct lcdDataStruct *lcd) {
#ifdef DEBUG
  fprintf(stderr, "lcdHome: lcdPutCommand(%d,%d)\n", lcd, LCD_HOME);
#endif
  lcdPutCommand(gpio, lcd, LCD_HOME);
  lcd->cx = lcd->cy = 0;
  delay(5);
}

void lcdClear (volatile uint32_t *gpio, struct lcdDataStruct *lcd) {
#ifdef DEBUG
  fprintf(stderr, "lcdClear: lcdPutCommand(%d,%d) and lcdPutCommand(%d,%d)\n", lcd, LCD_CLEAR, lcd, LCD_HOME);
#endif
  lcdPutCommand(gpio, lcd, LCD_CLEAR);
  lcdPutCommand(gpio, lcd, LCD_HOME);
  lcd->cx = lcd->cy = 0;
  delay(5);
}

void lcdPosition(volatile uint32_t *gpio ,struct lcdDataStruct *lcd, int x, int y) {
  // struct lcdDataStruct *lcd = lcds [fd] ;

  if ((x > lcd->cols) || (x < 0) ||
      (y > lcd->rows) || (y < 0)) {
    return;
  }

  lcdPutCommand(gpio, lcd, x + (LCD_DGRAM | (y>0 ? 0x40 : 0x00) 
        /* rowOff [y] */  )) ;

  lcd->cx = x;
  lcd->cy = y;
}

void lcdDisplay(volatile uint32_t *gpio ,struct lcdDataStruct *lcd, int state) {
  if (state) {
    lcdControl |= LCD_DISPLAY_CTRL;
  } else {
    lcdControl &= ~LCD_DISPLAY_CTRL;
  }

  lcdPutCommand(gpio, lcd, LCD_CTRL | lcdControl) ; 
}

void lcdCursor(volatile uint32_t *gpio, struct lcdDataStruct *lcd, int state) {
  if (state) {
    lcdControl |= LCD_CURSOR_CTRL;
  } else {
    lcdControl &= ~LCD_CURSOR_CTRL;
  }

  lcdPutCommand(gpio, lcd, LCD_CTRL | lcdControl);
}

void lcdCursorBlink(volatile uint32_t *gpio, struct lcdDataStruct *lcd, int state) {
  if (state) {
    lcdControl |= LCD_BLINK_CTRL;
  } else {
    lcdControl &= ~LCD_BLINK_CTRL;
  }

  lcdPutCommand(gpio, lcd, LCD_CTRL | lcdControl) ; 
}

void lcdPutchar(volatile uint32_t *gpio, struct lcdDataStruct *lcd,
    unsigned char data) {
  pinWrite(gpio, lcd->rsPin, 1);
  sendDataCmd(gpio, lcd, data);

  if (++lcd->cx == lcd->cols) {
    lcd->cx = 0;
    if (++lcd->cy == lcd->rows) {
      lcd->cy = 0;
    }

    // TODO: inline computation of address and eliminate rowOff
    lcdPutCommand(gpio, lcd, lcd->cx + (LCD_DGRAM | (lcd->cy>0 ? 0x40 : 0x00)
      /* rowOff [lcd->cy] */));
  }
}

/**
 * Send a string to be displayed on the display.
 */
void lcdPuts(volatile uint32_t *gpio, struct lcdDataStruct *lcd, const char *string) {
  while (*string) {
    lcdPutchar(gpio, lcd, *string++);
  }
}
