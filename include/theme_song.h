#include <Arduino.h>

#define NOTE_d4 426 // 293 Hz
#define NOTE_e4 379 // 329 Hz
#define NOTE_c4 479 // 261 Hz
#define NOTE_c3 961 // 130 Hz
#define NOTE_g3 637 // 196 Hz
#define NOTE_null 0 // pause

// Setup speaker
#define SPEAKERDDR DDRL // DDR for speaker
#define TIMER5_CONTROL_REG_A TCCR5A // speaker control register A
#define TIMER5_CONTROL_REG_B TCCR5B // speaker control register B
#define TIMER5_TOGGLE OCR5A // toggle bit for speaker, on pin D46 (PL3)

extern int theme_done; // flag for whether the theme has finished playing

void theme();
void silence();