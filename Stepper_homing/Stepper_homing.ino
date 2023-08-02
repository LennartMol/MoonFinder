// Include the AccelStepper library:
#include <AccelStepper.h>
#include <ezButton.h>
ezButton limitSwitch(7);  // create ezButton object on pin 7
// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1
#define homeButtonPin 8
#define startButtonPin 9

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
// 1, 3, 2, 

// Start button state
bool startButtonPressed = false;
bool homeButtonPressed = false;

int customPosition = 50;

void setup() {
  // Set the maximum speed and acceleration:
  stepper.setMaxSpeed(500);
  stepper.setSpeed(100);

  pinMode(homeButtonPin, INPUT_PULLUP);
  pinMode(startButtonPin, INPUT_PULLUP);
  Serial.begin(9600);
 
}

void loop() {
   //Serial.println(stepper.currentPosition());
   limitSwitch.loop();
   int limitSwitchPressed = !(limitSwitch.getState());
   //Serial.println(limitSwitchPressed);

   if((!homeButtonPressed) && (!digitalRead(homeButtonPin))) {
    homeButtonPressed = true;
    Serial.println("Home button pressed.");
   }

   // Only loop when start button pressed state is true
   if (homeButtonPressed) {
    if (!limitSwitchPressed){
      stepper.setSpeed(100);
      stepper.runSpeed();
    }
    else{
      stepper.setCurrentPosition(0);
      homeButtonPressed = false;
      Serial.println("Limit switch pressed.");
    }
   }

   if(!digitalRead(startButtonPin)) {
    Serial.println("Start button pressed.");
    stepper.moveTo(customPosition);
    stepper.runToNewPosition(customPosition);
   }
}
