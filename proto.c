// This is a prototype of a mastermind game
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define MAX_GUESS 12

enum Color {red,green,blue};
static const char Color_String[] = {'R', 'G', 'B'};

int main(int argc, char ** argv) {
  printf("MasterMind prototype\n");
  srand(time(NULL));
  enum Color secret[3] = {rand()%3, rand()%3, rand()%3};
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
    bool guessed[] = {false, false, false};
    // Exact = Exact guesses
    // Similar = Correct color, wrong position
    for(j=0;j<3;j++){    // Secret loop
      for(k=0;k<3;k++) { // Guess loop
        if(Color_String[secret[j]] == guess[k]){
          if(j==k){
            exact++;
          }else if(!guessed[j]){
            similar++;
            guessed[j] = true;
          }
        }
      }
    }

    printf("Answer %d:\t%d %d\n", i, exact, similar);
    if((exact+similar) >3){ // Hopefully my logic is sound
      printf("LOGIC ERROR: Sum of both number is greater than 3!\n");
    }
    if(exact == 3){
      printf("Game won in %d moves!\n", i);
      break;
    }
  }
  if(exact != 3)
    printf("Game over, you loose!\n");

}


