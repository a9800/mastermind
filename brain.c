#include "brain.h"

static volatile unsigned int gpiobase;
static volatile uint32_t *gpio;
struct lcdDataStruct *lcd;

int getBInput(int buttonPin, volatile unit32_t gpio ){
  int theValue = 0;

  // controlling button pin 19
  int fSel =  1;    
  int shift =  27;  
  *(gpio + fSel) = *(gpio + fSel) & ~(7 << shift); 
  
  while(theValue != 1){
    if ((buttonPin & 0xFFFFFFC0 /* PI_GPIO_MASK */) == 0){
	    theValue = 0;

      if ((*(gpio + 13 /* GPLEV0 */) & (1 << (buttonPin & 31))) != 0){
        theValue = 1;

      }else{
        theValue = 0;
      }

    }else{
      fprintf(stderr, "only supporting on-board pins\n");          
      exit(1);
    }
    struct timespec sleeper, dummy ;

    sleeper.tv_sec  = (time_t)(200 / 1000) ;
    sleeper.tv_nsec = (long)(200 % 1000) * 1000000 ;

    nanosleep (&sleeper, &dummy) ;
  }
  *(gpio + 7) = 1 << (23 & 31);

  return theValue;  
}

void initGPIO(){
  int   fd;
  if (geteuid() != 0){
    fprintf(stderr, "setup: Must be root. (Did you forget sudo?)\n");
    exit(9);
  }

  gpiobase = 0x3F200000;

  if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0) {
    fprintf(stderr, "mastermind: unable to open /dev/mem: %s\n", strerror(errno));
    exit(12);
  }
  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE,
      MAP_SHARED, fd, gpiobase);
  if (gpio == 0) {
    fprintf(stderr, "mastermind: mmap (GPIO) failed: %s\n", strerror(errno));
    exit(13);
  }

}

void lcdIntHandler(int i) {
  printf("Exiting... clearing LCD\n");
  lcdClear(gpio, lcd);
  printf("Goodbye!\n");
  exit(0);
}
void initLCD() {
  int i;
  int bits, rows, cols;
  unsigned char func;

  //struct tm *t;
  //time_t tim;
  //char buf [32];

  // hard-coded: 16x2 display, using a 4-bit connection
  bits = 4;
  cols = 16;
  rows = 2;

  printf("Raspberry Pi LCD driver, for a %dx%d display (%d-bit wiring) \n",
    cols, rows, bits);

  initGPIO();

  lcd = (struct lcdDataStruct *)malloc(sizeof(struct lcdDataStruct));
  if (lcd == NULL) {
    exit(-21);
  }

  // hard-wired GPIO pins
  lcd->rsPin   = RS_PIN;
  lcd->strbPin = STRB_PIN;
  lcd->bits    = 4;
  lcd->rows    = rows;  // # of rows on the display
  lcd->cols    = cols;  // # of cols on the display
  lcd->cx      = 0;     // x-pos of cursor
  lcd->cy      = 0;     // y-pos of curosr

  lcd->dataPins [0] = DATA0_PIN;
  lcd->dataPins [1] = DATA1_PIN;
  lcd->dataPins [2] = DATA2_PIN;
  lcd->dataPins [3] = DATA3_PIN;

  pinWrite (gpio, lcd->rsPin,   0); //pinMode (gpio, lcd->rsPin,   OUTPUT);
  pinWrite (gpio, lcd->strbPin, 0); //pinMode (gpio, lcd->strbPin, OUTPUT);

  for (i = 0; i < bits; ++i) {
    pinWrite (gpio, lcd->dataPins [i], 0);
    //pinMode      (gpio, lcd->dataPins [i], OUTPUT);
  }
  delay(35);

  if (bits == 4) {
    func = LCD_FUNC | LCD_FUNC_DL;     // Set 8-bit mode 3 times
    lcdPut4Command (gpio, lcd, func >> 4); delay (35);
    lcdPut4Command (gpio, lcd, func >> 4); delay (35);
    lcdPut4Command (gpio, lcd, func >> 4); delay (35);
    func = LCD_FUNC;         // 4th set: 4-bit mode
    lcdPut4Command (gpio, lcd, func >> 4); delay (35);
    lcd->bits = 4;
  } else {
    fprintf(stderr, "mastermind: only 4-bit connection supported\n");
    exit(44);
  }

  if (lcd->rows > 1) {
    func |= LCD_FUNC_N;
    lcdPutCommand (gpio, lcd, func); delay (35);
  }

  // Rest of the initialisation sequence
  lcdDisplay     (gpio, lcd, TRUE);
  lcdCursor      (gpio, lcd, FALSE);
  lcdCursorBlink (gpio, lcd, FALSE);
  lcdClear       (gpio, lcd);

  lcdPutCommand (gpio, lcd, LCD_ENTRY   | LCD_ENTRY_ID);    // set entry mode to increment address counter after write
  lcdPutCommand (gpio, lcd, LCD_CDSHIFT | LCD_CDSHIFT_RL);  // set display shift to right-to-left
  
  fprintf(stderr,"Printing welcome message ...\n");
  lcdPosition (gpio, lcd, 0, 0); lcdPuts (gpio, lcd, "Guess! :)   ");
  lcdPosition (gpio, lcd, 0, 1); lcdPuts (gpio, lcd, "            ");

  signal(SIGINT, lcdIntHandler);
  //lcdClear    (gpio, lcd);
}


int colortoint(char c) {
  switch(c){
    case 'R':
      return 0;
    case 'G':
      return 1;
    case 'B':
      return 2;
   }
  return 0;
}

int main(int argc, char ** argv) {
  initLCD();
  printf("MasterMind prototype\n");
  srand(time(NULL));
  //enum Color secret[3] = {rand()%3, rand()%3, rand()%3};
  enum Color secret[3] = {0,1,2};
  printf("Secret: %c  %c  %c\n", Color_String[secret[0]],
      Color_String[secret[1]], Color_String[secret[2]]);
  printf("==========\n");
  // Guess loop
  bool won = false;
  int i, exact, similar;
  for(i=1;i<=MAX_GUESS;i++) {
    exact = similar = 0;
    printf("Guess %d:\n", i);
    char guess[3];
    scanf(" %c %c %c", &guess[0], &guess[1], &guess[2]);

    int j,k;
    bool guessed[] = {false, false, false}; // R, G, B
    // Exact = Exact guesses
    // Similar = Correct color, wrong position
    for(j=0;j<3;j++){
      if(Color_String[secret[j]] == guess[j]){
        guessed[j] = true;
        exact++;
      }
    }
    for(j=0;j<3;j++){    // Secret loop
      for(k=0;k<3;k++) { // Guess loop
        if(j!=k && !guessed[colortoint(Color_String[k])] && Color_String[secret[j]] == guess[k]){
          similar++;
          break;
        }
      }
    }

    lcdClear       (gpio, lcd);
    char *exactStr=(char*)malloc(12 * sizeof(char));
    char *similarStr=(char*)malloc(12 * sizeof(char));
    sprintf(exactStr,"Exact  : %d",exact);
    sprintf(similarStr,"Similar: %d",similar);
    lcdClear       (gpio, lcd);
    lcdPosition (gpio, lcd, 0, 0); lcdPuts(gpio, lcd, exactStr);
    lcdPosition (gpio, lcd, 0, 1); lcdPuts(gpio, lcd, similarStr);
    printf("Answer %d:\t%d %d\n", i, exact, similar);
    if((exact+similar) >3){ // Hopefully my logic is sound
      printf("LOGIC ERROR: Sum of both number is greater than 3!\n");
    }
    if(exact == 3){
      char *message=(char*)malloc(12 * sizeof(char));
      sprintf(message,"Game won in %d", i);
      lcdClear(gpio, lcd);
      lcdPosition(gpio, lcd, 0, 0); lcdPuts(gpio, lcd, message);
      lcdPosition(gpio, lcd, 0, 1); lcdPuts(gpio, lcd, "moves!");
      printf("Game won in %d moves!\n", i);
      break;
    }
  }
  if(exact != 3){
    lcdClear(gpio, lcd);
    lcdPosition(gpio, lcd, 0, 0); lcdPuts(gpio, lcd, "Game over, you");
    lcdPosition(gpio, lcd, 0, 1); lcdPuts(gpio, lcd, "loose!");
    printf("Game over, you loose!\n");
  }
  
  printf("\nQuitting...\n");
  delay(3000);
  lcdClear(gpio, lcd);
}


