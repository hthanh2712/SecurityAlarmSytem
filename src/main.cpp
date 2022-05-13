#include "setup.h" 
#include <Arduino_FreeRTOS.h>

/**
 * @brief Initializes serial and creates RTOS tasks.
 * 
 */
void setup() {
  // initialize serial at 19200 bits per sec
  Serial.begin(19200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  } 

  // initialize the LCD
  lcd.begin(16,2);

  // initialize the external LED on pin 50 as output
  LEDDDR |= BIT_3;

  // initialize the buzzer on pin 8 as output
  BUZZERDDR |= BIT_5;

  // initialize the ultrasound sensor
  TRIGPIN_DDR |= BIT_6; // Set the trigPin on pin 9 as output
  //ECHOPIN_DDR &= BIT_4; // Set the echoPin on pin 10 as input
  pinMode(echoPin, INPUT); 

  // Now set up two tasks to run independently.
  xTaskCreate(
    Task_BlinkOffBoard
    ,  "Blink"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

/*
  xTaskCreate(
    Task_ActivateAlarm
    ,  "Activate the alarm"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    Task_CheckAlarm
    ,  "Check the distance and alarm state"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );
    */
/*
  xTaskCreate(
    handleLED
    ,  "Turn on alarm LED"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );
*/
/*
  xTaskCreate(
    handleBuzz
    ,  "Turn on buzzer"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );
*/
    vTaskStartScheduler();
}

/**
 * @brief Not used by FreeRTOS.
 * 
 */
void loop() {
  // empty
}

/**
 * @brief Task RT-1: Turns an off-board LED ON for 100 ms, then OFF for 200ms, repeatedly.
 * @param pvParameters Allocate stack space for parameters.
 */
void Task_BlinkOffBoard(void *pvParameters) {

  // initialize the external LED on pin D50 (PB3) as output
  LEDDDR |= BIT_3;

  for (;;) // A Task shall never return or exit.
  {
    LEDPORT |= BIT_3;   // turn the LED on 
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for 100 ms
    LEDPORT &= ~BIT_3;    // turn the LED off 
    vTaskDelay( 200 / portTICK_PERIOD_MS ); // wait for 200 ms
  }
}

/**
 * @brief Arms the alarm system. Displays countdown on LCD and triggers auditory and visual alarms 
 * if correct passcode is not entered during the countdown.
 * 
 * @param pvParameters Allocate stack space for parameters.
 */
void Task_ActivateAlarm(void *pvParameters) {
  for (;;) // A Task shall never return or exit.
  {
    if (activateAlarm) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Alarm will be");
      lcd.setCursor(0,1);
      lcd.print("activated in");
    
      // 5 seconds count down before activating the alarm
      int countdown = 5; 
      while (countdown != 0) {
        lcd.setCursor(13,1);
        lcd.print(countdown);
        countdown--;
        buzzerOn(); // buzzer produce a 1000Hz sound
        vTaskDelay(100/portTICK_PERIOD_MS); // wait for 100 ms 
        buzzerOff(); // turn on buzzer
        vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for 1000 ms
      }
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Alarm Activated!");
      initialDistance = getDistance();
      activateAlarm = false;
      alarmActivated = true;
    }
  }
}

/**
 * @brief Polls the ultrasonic sensor to determine if there is something "close" to the sensor. Triggers password request when approached.
 * If alarm is not activate, choose between two options - activate or change passcode.
 * 
 * @param pvParameters Allocate stack space for parameters.
 */
void Task_CheckAlarm(void *pvParameters) {
  for (;;) // A Task shall never return or exit.
  {
    if (alarmActivated == true){
      currentDistance = getDistance() + 10;
      if (currentDistance < initialDistance) {
        lcd.clear();
        enterPassword();
      }
    }

    if (!alarmActivated) {
      if (screenOffMsg == 0 ){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("A - Activate");
        lcd.setCursor(0,1);
        lcd.print("B - Change Pass");
        screenOffMsg = 1;
      }
      keypressed = myKeypad.getKey();
      
      if (keypressed =='A'){  // If A is pressed, activate the alarm
        activateAlarm = true;            
      } else if (keypressed =='B') { // If B is pressed, process to change password
        lcd.clear();
        int i=1;
        tempPassword = "";
        lcd.setCursor(0,0);
        lcd.print("Current Password");
        lcd.setCursor(0,1);
        lcd.print(">");
        passChangeMode = true;
        passChanged = true;   
        
        while(passChanged) { changePassword(); }
      }
    }
  }
}

/**
 * @brief Initialize the alarm's LED. If the alarm is activated, turn on the LED.
 * 
 * @param pvParameters Allocate stack space for parameters.
 */
void handleLED(void *pvParameters) {

  for (;;) // A Task shall never return or exit.
  {
    if (alarmActivated == true) {
      //Serial.println("setting LED HIGH");
      LEDPORT |= BIT_5;   // turn the LED on 
    } else {
      //Serial.println("setting LED LOW");
      LEDPORT &= ~BIT_5;    // turn the LED off 
    }
  }  
}

/**
 * @brief Changes the passcode via the keypad.
 * 
 */
void changePassword() {   
  keypressed = myKeypad.getKey();
    
  if (keypressed != NO_KEY) 
    {
      if (keypressed == '0' || keypressed == '1' || keypressed == '2' || keypressed == '3' ||
          keypressed == '4' || keypressed == '5' || keypressed == '6' || keypressed == '7' ||
          keypressed == '8' || keypressed == '9' ) 
      {
        tempPassword += keypressed;
        lcd.setCursor(i,1);
        lcd.print("*");
        i++;
        buzzerOn(); // buzzer produce a 1000Hz sound
        vTaskDelay(100/portTICK_PERIOD_MS); // wait for 100 ms 
        buzzerOff(); // turn on buzzer
      }
    }
    
    if (i > 5 || keypressed == '#') 
    {
      tempPassword = "";
      i=1;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Current Password");
      lcd.setCursor(0,1);
      lcd.print(">"); 
    }
    
    if ( keypressed == '*') 
    {
      i=1;
      buzzerOn(); // buzzer produce a 1000Hz sound
      vTaskDelay(100/portTICK_PERIOD_MS); // wait for 100 ms 
      buzzerOff(); // turn on buzzer
      if (password == tempPassword) 
      {
        tempPassword="";
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set New Password");
        lcd.setCursor(0,1);
        lcd.print(">");
        while(passChangeMode) 
        {
          keypressed = myKeypad.getKey();
          if (keypressed != NO_KEY)
          {
            if (keypressed == '0' || keypressed == '1' || keypressed == '2' || keypressed == '3' ||
                keypressed == '4' || keypressed == '5' || keypressed == '6' || keypressed == '7' ||
                keypressed == '8' || keypressed == '9' ) {
              tempPassword += keypressed;
              lcd.setCursor(i,1);
              lcd.print("*");
              i++;
              buzzerOn(); // buzzer produce a 1000Hz sound
              vTaskDelay(100/portTICK_PERIOD_MS); // wait for 100 ms 
              buzzerOff(); // turn on buzzer
            }
          }
          if (i > 5 || keypressed == '#') {
            tempPassword = "";
            i=1;
            buzzerOn(); // buzzer produce a 1000Hz sound
            vTaskDelay(100/portTICK_PERIOD_MS); // wait for 100 ms 
            buzzerOff(); // turn on buzzer
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Set New Password");
            lcd.setCursor(0,1);
            lcd.print(">");
          }
          if ( keypressed == '*') {
            i=1;
            buzzerOn(); // buzzer produce a 1000Hz sound
            vTaskDelay(100/portTICK_PERIOD_MS); // wait for 100 ms 
            buzzerOff(); // turn on buzzer
            password = tempPassword;
            passChangeMode = false;
            passChanged = false;
            screenOffMsg = 0;
          }            
        }
      }
    }
}

/**
 * @brief Get the measured distance from the ultrasonic sensor.
 * 
 * @return long variable distance
 */
long getDistance(){
  //int i=10;
  
  //while( i<=10 ) {
  // Clears the trigPin
  TRIGPIN_PORT &= ~BIT_6;
  vTaskDelay( 2 / portTICK_PERIOD_MS );

  // Sets the trigPin on HIGH state for 10 micro seconds
  TRIGPIN_PORT |= BIT_6;
  vTaskDelay( 10 / portTICK_PERIOD_MS );
  TRIGPIN_PORT &= ~BIT_6;

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  distance = duration*0.034/2;
  //sumDistance += distance;
  //}
  //int averageDistance= sumDistance/10;
  return distance;
}

/**
 * @brief Handles password entry via the keypad.
 * 
 */
void enterPassword() {
  int k=5;
  tempPassword = "";
  activated = true;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" *** ALARM *** ");
  lcd.setCursor(0,1);
  lcd.print("Pass>");
  while(activated) {
    keypressed = myKeypad.getKey();
    if (keypressed != NO_KEY){
        if (keypressed == '0' || keypressed == '1' || keypressed == '2' || keypressed == '3' ||
            keypressed == '4' || keypressed == '5' || keypressed == '6' || keypressed == '7' ||
            keypressed == '8' || keypressed == '9' ) {
          tempPassword += keypressed;
          lcd.setCursor(k,1);
          lcd.print("*");
          k++;
        }
    }
    if (k > 9 || keypressed == '#') {
        tempPassword = "";
        k=5;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(" *** ALARM *** ");
        lcd.setCursor(0,1);
        lcd.print("Pass>");
    }
    if ( keypressed == '*') {
      if ( tempPassword == password ) {
          activated = false;
          alarmActivated = false;
          buzzerOff();
          screenOffMsg = 0; 
      }
      else if (tempPassword != password) {
          lcd.setCursor(0,1);
          lcd.print("Wrong password! Try Again");
          vTaskDelay( 1000 / portTICK_PERIOD_MS );
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print(" *** ALARM *** ");
          lcd.setCursor(0,1);
          lcd.print("Pass>");
      }
    }    
  }
}

/**
 * @brief Initialize the buzzer. If the alarm is activates and the distance is closer,
 * turn on the buzzer until entering correct password.
 * 
 * @param pvParameters Allocate stack space for parameters.
 */
void handleBuzz(void *pvParameters) {
  for (;;) // A Task shall never return or exit.
  {
    if (alarmActivated == true){
      currentDistance = getDistance() + 10;
      if (currentDistance < initialDistance) {
        buzzerOn(); // buzzer produce a 1000Hz sound
      }
    }
  }
}

/**
 * @brief Turns the buzzer on when called.
 * 
 */
void buzzerOn() {
  TIMER4_TOGGLE = 124; // should set buzzer to generate 1000Hz tone
  BUZZERDDR |= BIT_5;
}

/**
 * @brief Turns the buzzer off when called.
 * 
 */
void buzzerOff() {
  BUZZERDDR = 0;
}