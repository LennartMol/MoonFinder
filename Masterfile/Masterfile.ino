#include <math.h>
#include <Servo.h>
#include <ezButton.h>
#include <Stepper.h>
#include <AccelStepper.h>
#include <SoftwareSerial.h>
#include <neotimer.h>

// Communication between ESP32 and Arduino
#define DATA_FROM_ESP_PIN 10
SoftwareSerial mySerial(DATA_FROM_ESP_PIN, -1); // RX, TX - only RX is used

// Global variables calculations
const int stepsPerRevolution = 200;// Define number of steps per rotation:
float DegPerStep = 360.0/stepsPerRevolution; //Calculates how many degrees per step it moves.
int Xsteps = 0;
int Degrees = 0;
int OldDegrees= 0;
float Angle = 0;
float DayPerRev = 27.3;
int t = 0;
float T = 0; //Is equal to 
float X1 = 0; //Is the cosine of the circle
float Y1 = 0; //Is the sinus of the circle
float AVMoon = PI*2/27.3; //Angular velocity of the moon around the Earth.
float Offset = 0.497419; //difference between plane of rotation Earth, relative to the plane of ration Moon.
int Interval = 128; //define amount of updates per day
float DeltaT = DayPerRev/Interval;
float Amp = (sin(DayPerRev/4*AVMoon)*cos(Offset)); //Calculates the Amplitude of the sinewave
float moonTime = 0;

// limit switch
ezButton limitSwitch(7);  // create ezButton object on pin 7
// Start button state

#define homeButtonPin 8
#define startButtonPin 9

bool homeButtonPressed = false;
bool startButtonPressed = false;

Neotimer mytimer = Neotimer(500); 

int test = 0;

// Wiring:

// GND to ESP32 GND
// Pin 10 to ESP32 TX0 (tussen RX0 en D22) 

// Pin 8 to IN1 on the ULN2003 driver
// Pin 9 to IN2 on the ULN2003 driver
// Pin 10 to IN3 on the ULN2003 driver
// Pin 11 to IN4 on the ULN2003 driver

// Stepper object
#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

// Servo object
Servo myservo;  
float pos = 0;    // variable to store the servo position
float var = 0;

// parameter: dateTime in string format
// format: YYYYMMDDHHMMSS
// example: date=2022-12-19, time=20:19:14 -> 20221219201914
float timeToSideReal(String time){
  // time constants
  float Psyn = 29.5305888531; // Moon Synodic days (0 to 29.5305888531)
  float Pyear = 365.2421897; // Period of earth 
  
  // weird magic number
  float magicNumber = -1.211249; 

  // get date variables
  float hours = time.substring(11,13).toFloat();
  float minutes = time.substring(14,16).toFloat();
  float seconds = time.substring(17,19).toFloat();
  float year = time.substring(0,4).toFloat();
  float month = time.substring(5,7).toFloat();
  float day = time.substring(8,10).toFloat();
  
  // get time in percentage of 24 hours
  float timePercentageOf24Hours = (hours+(minutes/60)+(seconds/3600))/24;
  
  // arduino shenanigans of converting to julian time
  float JD1 = (year+4800+(month-14)/12);
  float JD2 = (month-2-(month-14)/12*12);
  float JD3 = ((year+4900+(month-14)/12)/100);
  
  // split JD4 into whole number and decimals into different variables
  float JD4 = (1461*JD1/4) * 1000;
  float JD4_whole = floor(JD4 / 1000);
  
  unsigned long JD4_long = (1461*JD1/4) * 1000;
  String JD4_whole_String = String(JD4_long);
  float JD4_decimals = JD4_whole_String.substring(7,10).toFloat() / 1000;
  
  float JD5 = (367*JD2/12);
  float JD6 = (3*JD3/4);
  
  // calculate whole day first to prevent float overflow rounding 
  float JD_whole = day-32075 + JD4_whole + JD5 - JD6;
  
  // subtract julian dates and later add decimals to prevent overflow again
  float JD = (JD_whole - 2451550.1) + JD4_decimals + timePercentageOf24Hours + magicNumber;
  
  // calculate moon age in synodic notation
  float IP = fmodf((JD / Psyn), 1);
  float moonAgeSynodic = IP*Psyn;
  
  // calculate moon age in side real notation - this is your T
  float moonAgeSideReal = 1/((1/moonAgeSynodic)+(1/Pyear)); 
  
  return moonAgeSideReal;
}

// parameter: dateTime in string format
// format: YYYYMMDDHHMMSS
// example: date=2022-12-19, time=20:19:14 -> 20221219201914
bool checkForCurruptData(String time){
  int dateTimeLength = 14;
  
  // check if length is not too short or too long
  if (time.length() != dateTimeLength){
    return false;
  }
  // check if all characters are digits, so not currupt data is present
  for(int i = 0; i < dateTimeLength -1; i ++){
    if(!isDigit(time[i])){
      return false;
    }
  }
  return true;
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(115200);
  myservo.attach(4);  // attaches the servo on pin 12 to the servo object

  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  stepper.setSpeed(20);

  pinMode(homeButtonPin, INPUT_PULLUP);
  pinMode(startButtonPin, INPUT_PULLUP);

  mytimer.set(1000); // set to 1 second
}

void loop() {
  stepper.run();
  if(mytimer.repeat()) {
    // get time
    // Receive data from ESP32
    if (mySerial.available()) {
      String dateTime = mySerial.readString();
      dateTime.trim();
      if(checkForCurruptData(dateTime)){
        Serial.println("Correct date and time received: " + dateTime);
        moonTime = timeToSideReal(dateTime);
        Serial.println("moonAgeSideReal: " + String(moonTime));
      }
      else{
        Serial.println("Incorrect date and time received: " + dateTime);
      }
    }
    else {
      Serial.println("No data received from ESP32");
    }
  }
  
  
  // homing shit

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


  // start everything
  if(!digitalRead(startButtonPin)) {
    Serial.println("Start button pressed.");

    //Servo code
  pos = 28.5 * sin(27.3/moonTime);
  pos = pos +90;
  Serial.print("moonTime: ");
  Serial.println(moonTime);
  Serial.print("pos: ");
  Serial.println((int)pos);
  myservo.write(pos);

  //Stepper code

  T = moonTime;
  X1 = -cos(AVMoon*T);
  Y1 = sin(AVMoon*T)*Amp;

  Angle = (atan2(Y1,X1));

  Degrees = ((Angle * 4068) / 71) + 180;


  Xsteps = (Degrees-OldDegrees)/DegPerStep;
  Serial.print("T: ");
  Serial.println(T); 
  Serial.print("Y1: ");
  Serial.println(Y1); 
  Serial.print("X1: ");
  Serial.println(X1);
  Serial.print("Degrees: ");
  Serial.println(Degrees);
  Serial.print("Xsteps: ");
  Serial.println(Xsteps);
  stepper.moveTo(Xsteps);
  OldDegrees = Degrees;  
  
  delay(1000);
  }
}
