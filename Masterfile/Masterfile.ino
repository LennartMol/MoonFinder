#include <math.h>
#include <Servo.h>
#include <ezButton.h>
#include <Stepper.h>
#include <AccelStepper.h>
#include <SoftwareSerial.h>
#include <neotimer.h>
#include <TimeLib.h>


// Global variables calculations
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

Neotimer mytimer = Neotimer(500); 

// Wiring:

// Communication between ESP32 and Arduino
// GND to ESP32 GND
// Pin 10 to ESP32 TX0 (tussen RX0 en D22) 
#define DATA_FROM_ESP_PIN 10
SoftwareSerial mySerial(DATA_FROM_ESP_PIN, -1); // RX, TX - only RX is used

// Start button 
#define startButtonPin 9
bool startButtonPressed = false;

// Servo pins
#define servoBasePin 5
#define servoArrowPin 4

// Servo base
Servo servoBase;  
float posServoBase = 0;    // variable to store the servo position

// Servo arrow
Servo servoArrow;  
float posServoArrow = 0;    

long convertToJulianDate(String time) {
  // Extracting components from the input string
  float year = time.substring(0, 4).toFloat();
  float month = time.substring(4, 6).toFloat();
  float day = time.substring(6, 8).toFloat();
  float hour = time.substring(8, 10).toFloat();
  float minute = time.substring(10, 12).toFloat();
  float second = time.substring(12, 14).toFloat();


  // Creating a tmElements_t structure to hold the date and time
  tmElements_t dateTime;
  dateTime.Year = year - 1970; // TimeLib starts from year 1970
  dateTime.Month = month;
  dateTime.Day = day;
  dateTime.Hour = hour;
  dateTime.Minute = minute;
  dateTime.Second = second;

  // Convert to Unix time
  time_t unixTime = makeTime(dateTime);

  // Convert Unix time to Julian date
  long julianDate = (unixTime / 86400.0) + 2440587.5;

 // Print the result
  Serial.print("Julian date: ");
  Serial.println(julianDate);



  return julianDate;
}

float convertToFractionTime(String time) {
  float hours = time.substring(8, 10).toFloat();
  float minutes = time.substring(10, 12).toFloat();
  float seconds = time.substring(12, 14).toFloat();

  long hoursInSeconds = (long)hours * 3600L;

  long minutesInSeconds = (long)minutes * 60L;

  long totalSeconds = hoursInSeconds + minutesInSeconds + seconds;

  long totalSecondsInDay = 24L * 3600L;

  float fraction = (float)totalSeconds / (float)totalSecondsInDay;

  // Print the result
  Serial.print("Fraction of the day: ");
  Serial.println(fraction, 6);

  return fraction;
}

float calculateTFromHolyTime(String datetime) {
  String HeiligeJulianDate = "2024040812200";
  String date = datetime;
  long current = convertToJulianDate(datetime);
  float currentT = convertToFractionTime(datetime);
  long holy = convertToJulianDate(HeiligeJulianDate);
  float holyT = convertToFractionTime(HeiligeJulianDate);
  int dif = current - holy;
  if (currentT < holyT) {
    currentT += 1;
  }
  float difT = currentT - holyT;
  Serial.print("Difference in days: ");
  Serial.println(dif);
  Serial.print("Difference in time: ");
  Serial.println(difT, 6);

  // add the day and time difference 
  float result = dif + difT;

  // use modula 27.3 to get the result
  result = fmod(result, 27.3);
  Serial.print("T: ");
  Serial.println(result, 6);

  return result;
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
  servoBase.attach(servoBasePin);  
  servoArrow.attach(servoArrowPin);

  pinMode(startButtonPin, INPUT_PULLUP);

  mytimer.set(1000); // set to 1 second
}

void loop() {

  if(mytimer.repeat()) {
    // get time
    // Receive data from ESP32
    if (mySerial.available()) {
      String dateTime = mySerial.readString();
      dateTime.trim();
      //dateTime = "20250106122000";
      if(checkForCurruptData(dateTime)){
        Serial.println("Correct date and time received: " + dateTime);
        moonTime = calculateTFromHolyTime(dateTime);
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


  // start everything
  if(!digitalRead(startButtonPin)) {
    Serial.println("Start button pressed.");

  
  
  //Servo code
  posServoArrow = 28.5 * sin(27.3/moonTime);
  posServoArrow = posServoArrow + (270/2);
  Serial.print("moonTime: ");
  Serial.println(moonTime);
  Serial.print("pos Arrow servo: ");
  Serial.println((int)posServoArrow);
  servoArrow.write(posServoArrow);

  T = moonTime;
  X1 = -cos(AVMoon*T);
  Y1 = sin(AVMoon*T)*Amp;

  Angle = (atan2(Y1,X1));

  Degrees = ((Angle * 4068) / 71) + 180;


  Serial.print("pos Base servo (before map): ");
  Serial.println((int)posServoBase);
  Degrees = map(Degrees, 0, 360, 0, 270);


  posServoBase = Degrees;
  servoBase.write(posServoBase);
  Serial.print("pos Base servo: ");
  Serial.println((int)posServoBase);

  Serial.print("T: ");
  Serial.println(T); 
  Serial.print("Y1: ");
  Serial.println(Y1); 
  Serial.print("X1: ");
  Serial.println(X1);
  Serial.print("Degrees: ");
  Serial.println(Degrees);

  OldDegrees = Degrees;  
  
  delay(1000);
  }
}
