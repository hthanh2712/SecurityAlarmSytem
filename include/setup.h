#include <Arduino.h>
#include <arduinoFFT.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <Arduino_FreeRTOS.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue.h>
#include <task.h>
#include <timers.h>

/*
 * Port manipulation setup
*/

// define bits
#define BIT_7 (1<<7)
#define BIT_6 (1<<6)
#define BIT_5 (1<<5)
#define BIT_4 (1<<4) 
#define BIT_3 (1<<3) 
#define BIT_2 (1<<2) 
#define BIT_1 (1<<1) 
#define BIT_0 (1<<0)

// setup off-board LED in pin D50 (PB5)
#define LEDPORT PORTB  // PORT for LED
#define LEDDDR DDRB    // DDR for LED

// Setup buzzer
#define BUZZERDDR DDRH // DDR for buzzer
#define TIMER4_CONTROL_REG_A TCCR4A // buzzer control register A
#define TIMER4_CONTROL_REG_B TCCR4B // buzzer control register B
#define TIMER4_TOGGLE OCR4C // toggle bit for buzzer

// additional setups for home security system
#define TRIGPIN_PORT PORTH  // PORT for sensor
#define TRIGPIN_DDR DDRH    // DDR for sensor

#define ECHOPIN_PORT PORTB 
#define ECHOPIN_DDR DDRB   
#define echoPin 10
////////////////////////////////////////////////////////////////////////

// define tasks for Blink & AnalogRead
extern void TaskBlink_OffBoard( void *pvParameters ); // task RT-1
extern void TaskPlayingTone(void *pvParameters);      // task RT-2
extern void RT3p0(void *pvParameters);
extern void RT3p1(void *pvParameters);
extern void RT4(void *pvParameters);

////////////////////////////////////////////////////////////////////////

/*
 * Setup for security and alarm system project 
*/

long duration;
int distance, initialDistance, currentDistance, i;
int screenOffMsg =0;
String password="0000";
String tempPassword;
boolean activated = false; // State of the alarm
boolean isActivated;
boolean activateAlarm = false;
boolean alarmActivated = false;
boolean enteredPassword; // State of the entered password to stop the alarm
boolean passChangeMode = false;
boolean passChanged = false;

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keypressed;

//define the cymbols on the buttons of the keypads
char keyMap[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {14, 15, 16, 17}; // Row pinouts of the keypad
byte colPins[COLS] = {18, 19, 20, 21}; // Column pinouts of the keypad

Keypad myKeypad = Keypad( makeKeymap(keyMap), rowPins, colPins, ROWS, COLS); 
LiquidCrystal lcd(1, 2, 4, 5, 6, 7); // Creates an LC object. Parameters: (rs, enable, d4, d5, d6, d7) 

// define task RT-1 for Blink 
void Task_BlinkOffBoard(void *pvParameters); 

// define tasks for home security system project
void Task_ActivateAlarm(void *pvParameters); 
void Task_CheckAlarm(void *pvParameters);
void handleLED(void *pvParameters);
void handleBuzz(void *pvParameters);
void enterPassword(); 
void changePassword();
long getDistance();
void buzzerOn();
void buzzerOff();
