#include "theme_song.h"

int theme_done = 0; // flag for whether the theme has finished playing

// iterate through notes of theme song
void theme() {
  static int index = 0;
  const int theme_len = 9;
  int theme[] = {NOTE_d4, NOTE_e4, NOTE_c4, NOTE_c3, NOTE_g3, NOTE_g3, NOTE_g3, NOTE_g3, NOTE_null};
  if (index >= theme_len) {
    index = 0;
    theme_done = 1;
    }
  if (theme_done == 0) { TIMER5_TOGGLE = theme[index++]; }
  else { index = 0; }
}

void silence() {
  SPEAKERDDR = 0; // set speaker pin to input so that nothing is output
}
