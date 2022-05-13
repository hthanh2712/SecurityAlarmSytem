# SecurityAlarmSytem

Final academic project for ECE/CSE 474

## Part I: Implement a series of tasks applying FreeRTOS to create a pre-emptive scheduler

### Blinking an External LED 
* Connect a LED to pin D50.
#### Reading Analog Inputs from an Analog Thumbstick
* Connect a thumbstick to pin A1
* Receive the x-value data of the thumbstick via analog pin A1.
* Use the Serial Monitor to watch the x-value data as it was received.
### Play the "Close Encounters" theme song
* Connect the speaker to pin D46.
* Plays the tune from the movie “Close Encounter of the Third Kind” using Timer4 interrupt.
* Play and then sleep for N seconds, and then suspend itself at the end.
### Measure the Fast Fourier Transform
* Compute the Fast Fourier Transform five times as a background task.

## Part II: Security Alarm System

### Desciption

The components used in this project included the previously implemented off-board LED, as well as a piezo buzzer, ultrasonic sensor to detect movement in the proximity of the device by frequencies, liquid crystal display (LCD) to show alarm state and show input from the keypad, and membrane 4x4 keypad to arm and disarm the security system. A potentiometer was also added so that the LCD’s contrast can be easily varied if it is hard to read. Alarm with audible alert from an active buzzer and visual alert via an off-board LED.

### How It Works
To activate the alarm, the button A needs to be pressed. For detecting an object, the system uses an ultrasonic sensor. Once the alarm detects something, a buzzer will start emitting a sound of 1000 Hz frequency. 

In order to stop the alarm, we need to insert a 4-digit passcode. If we press the wrong numbers, the display will show a message that we need to try again. 

The default passcode is set as “0000” and we can change that to any other. When we want to change the passcode, we press the button B to enter the change menu. We first need to enter the current password. If that is correct, the process will move forward and we can enter our new password as desired. Once the new passcode is set, the next time the alarm is activated, we will only be able to stop the alarm by entering the new passcode.
