#include "setup.h"
#include "theme_song.h"

static QueueHandle_t xQueue1, xQueue2; ///< Queue handles
TaskHandle_t xHandle; ///< Task 3p0 handle
TaskHandle_t RT3p1Handle; ///< Task 3p1 handle
TaskHandle_t RT4Handle; ///< Task 4 Handle

static int N = 10;

void setup() {
  // initialize serial at 19200 bits per sec
  Serial.begin(19200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  } 

  // Now set up tasks to run independently.
  xTaskCreate(
    TaskBlink_OffBoard
    ,  "Blink"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  0  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    TaskPlayingTone
    ,  "Song" // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority
    ,  NULL );
 
  /// creates a task for RT3p0 with stack size 800, priority 3
  xTaskCreate(
    RT3p0
    ,  "RT3p0" // A name just for humans
    ,  800  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  &N
    ,  3  // Priority
    ,  &xHandle);
  /// creates a task for RT3p1 with stack size 800, priority 2
  xTaskCreate(
    RT3p1
    ,  "RT3p1" // A name just for humans
    ,  800  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority
    ,  &RT3p1Handle);
  /// creates a task for RT4 with stack size 1600, priority 1
  xTaskCreate(
    RT4
    ,  "Compute FFT" // A name just for humans
    ,  1600  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority
    ,  &RT4Handle);
    
 //vTaskSuspend(RT3p0Handle);
//  vTaskSuspend(RT3p1Handle);
//  vTaskSuspend(RT4Handle);
  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
  delay(500);
  vTaskStartScheduler();
}

void loop() {
  // empty
}

/**
 * @brief Task RT-1: Blink offboard LED at a rate of 100ms on and 200ms off, repeatedly.
 * 
 * @param pvParameters Allocate stack space for parameters.
 */
void TaskBlink_OffBoard(void *pvParameters) {

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
 * @brief Task RT-2: Play "Close Encounters Theme three times with 1.5s pause between playbacks."
 * 
 * @param pvParameters Allocate stack space for parameters.
 */
void TaskPlayingTone(void *pvParameters) {

  // setup speaker on pin D46
  SPEAKERDDR = 0; // clear port L
  SPEAKERDDR |= BIT_3; // set speaker pin as output
  TIMER5_CONTROL_REG_A = 0; // clear the register
  TIMER5_CONTROL_REG_B = 0; // clear the register
  TIMER5_CONTROL_REG_A |= BIT_6; // set COM5A0 so compare match toggles speaker
  TIMER5_CONTROL_REG_B |= ( BIT_3 | BIT_1 | BIT_0 ); // set timer to CTC with prescaler of 64

  for (int i = 0; i < 3; i++) // stop after 3 times playing
  { // stop after 3 times playing
      // play the theme (pause 1.5 sec between playbacks)
      // use vTaskDelay( 100 / portTICK_PERIOD_MS )?
      if (!theme_done) {
        theme();
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }
      if (theme_done) {
        silence();
        vTaskDelay(1500 / portTICK_PERIOD_MS);
      } else {SPEAKERDDR |= BIT_3;} // make sure speaker is on
  }
  vTaskSuspend(NULL); // stop this task
}

/**
 * @brief Task RT-3p0: Generates an array of N pseudorandom doubles, initializes a FreeRTOS queue, then calls Task RT-3p1.
 * 
 * @param pvParameters Allocate stack space for parameters. 
 */
void RT3p0(void *pvParameters) {
  vTaskSuspend(RT3p1Handle);
  vTaskSuspend(RT4Handle);
  
  int size1 = (int) *((int *)pvParameters);
  
  // generate an array of N pseudo-random doubles
  double signal[N];
  
  for (int i = 0; i < size1; i++) {
    signal[i] = drand((double)-230000000,(double)230000000);
  }

  // create a queue capable of containing N pseudo-random doubles
  xQueue1 = xQueueCreate( size1, sizeof(double));
  
  double * point = signal;
  
  //Serial.println(*(point + 1));
  vTaskDelay( 1500 / portTICK_PERIOD_MS );
  
  // send a pointer to the data to task RT-4
  xQueueSend( xQueue1, ( void * ) &point, ( TickType_t ) 0);
  
  // starts RT3p1
  vTaskResume(RT3p1Handle);
  delay(500);
  
  // stop this task
  vTaskSuspend(xHandle);
}

/**
 * @brief Generate a random number between the parameter bounds.
 * 
 * @param min Minimum value to output.
 * @param max Maximum value to output.
 * @return double Randomly generated number between min and max.
 */
double drand(double min, double max) {
    double range = (max - min); 
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

/**
 * @brief Task RT3p1: Blocks other tasks and signals for the FFT to run.
 * 
 * @param pvParameters Allocates stack space for parameters.
 */
void RT3p1 (void *pvParameters)
{ 
  vTaskSuspend(xHandle);
  vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one 600 ms
  TickType_t *computeTime;
  TickType_t total;
  
  // create a queue capable of containing N pseudo-random doubles
  xQueue2 = xQueueCreate( 5, sizeof( unsigned long ) );
  
  unsigned long wall_clock_time = 0;
  
  for (int i = 0; i < 5; i++)
  {
    // receive signal back from RT-4
    xQueueReceive( xQueue2, &( computeTime ), ( TickType_t ) 10 );
    vTaskResume(RT4Handle);
    
    while (xQueueReceive( xQueue2, &( computeTime ), ( TickType_t ) 10 ) == pdTRUE) {
      vTaskDelay( 500 / portTICK_PERIOD_MS ); // wait for 1500 ms
    }
    wall_clock_time += (unsigned long) computeTime;
    if (i == 4) {
      // report “wall clock time” elapsed for the 5 FFTs
      Serial.println(wall_clock_time);
    }
    delay(200);
  }

  vTaskSuspend(RT3p1Handle);
  
//  TickType_t * data;
//  
//  while (xQueueReceive( xQueue2, &( data ), ( TickType_t ) 10 ) == pdFALSE) { // if it receives from queue, a new value has been sent and unblock RT3
//    vTaskSuspend(RT3p1Handle);
//  }
  
  for (int i = 0; i < 5; i++)
  {
    //Serial.println((*(data + 1) * portTICK_PERIOD_MS));
  }

}

// create FFT object
arduinoFFT FFT = arduinoFFT();


/**
 * @brief Calculate FFT and report the "wall-clock time" taken by the FFT.
 * 
 * @param pvParameters Allocates stack space for parameters.
 */
void RT4 (void *pvParameters) {
//
//  vTaskDelay( 600 / portTICK_PERIOD_MS ); // wait for one 600 ms
//  int size1 = (int) *((int *)pvParameters);
//
//  double arr [10];
//  xQueue1 = xQueueCreate( size1, sizeof( double ) );
//  for (int i = 0; i < size1; i++)
//  {
//    arr[i] = (double)1;
//  }
//  double * point = arr;
//
//  xQueue2 = xQueueCreate( 5, sizeof( unsigned long ) );
//
//  
  vTaskSuspend(xHandle);
  const uint16_t samples = 64;
  const double samplingFrequency = 5000;
  double * dPointer;
  double vImag[samples];
  //Serial.print("test3");
  
  // receive data pointer from the first queue
  xQueueReceive(xQueue1, &( dPointer ), ( TickType_t ) 10 );
  for (uint16_t i = 0; i < samples; i++)
  {
    vImag[i] = 0.0; // imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
  }
  unsigned long start = millis();
  
  arduinoFFT FFT = arduinoFFT(dPointer, (double*) 0, samples, samplingFrequency);
  
  // compute FFT
  FFT.Compute(dPointer, vImag, samples, FFT_FORWARD);
  
  unsigned long finish = millis();
  unsigned long difference = finish - start;

  // send a message back to RT-3
  xQueueSend( xQueue2, &difference , ( TickType_t ) 0 );
  //vTaskResume(RT3p1Handle);
  FFT = arduinoFFT();
  delay(500);
  
  // stop the task
  vTaskSuspend(RT4Handle); 
  
}

