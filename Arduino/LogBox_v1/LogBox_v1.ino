//Arduino 1.0+ Only
//Arduino 1.0+ Only

#include "Wire.h"
#define DS1307_ADDRESS 0x68
#define DISPLAY_ADDRESS1 0x71 //This is the default address of the OpenSegment with both solder jumpers open

int second;
int minute;
int hour;
int currentTime;

boolean stopWatchRunning = false;
int stopWatchSecond = 0;
int stopWatchMinute = 0;
int stopWatchHour = 0;
int currentSecond;
int previousSecond;

const int buttonGRN = 15; //green button's output connection
const int buttonRED = 14; //red button's output connection
const int ledBRED = 7;  //the red button's LED connection
const int ledBGRN = 8;  //the green button's LED connection

long menuHoldCounter = 0; //variable for counting how long the green button has been held
const int menuHoldTime = 3000;
int modeSelect = 1; //the starting value that sets the starting function state

unsigned long previousMillis = 0;

void setup(){
  Wire.begin(); //Join the bus as master
  Serial.begin(9600); //Start serial communication at 9600 for debug statements
  Serial1.begin(9600); //Start serial communication on the Pro Micro's TXO and RXI

  pinMode(ledBGRN, OUTPUT);
  pinMode(ledBRED, OUTPUT);

  clearDisplay(); //Send the reset command to the display - this forces the cursor to return to the beginning of the display
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write(0x77);  // Decimal control command
  Wire.write(0b00000010);  // Turns on the second decimal
  Wire.endTransmission();
  delay(100);
}

void loop(){
  //Serial.print("Main Loop Current Mode: "); //debug serial print the RTC's date readings
  //Serial.println(modeSelect);
  unsigned long currentMillis = millis();  //save the milli time at the start of the loop

  //save the amount of time the green button is held down
  if (digitalRead(buttonGRN)){
    menuHoldCounter += (currentMillis - previousMillis);
  }
  else {
    menuHoldCounter = 0;
  }

  //if the green button is held down past menuHoldTime toggle the menu boolean
  if (menuHoldCounter > menuHoldTime){
    modeSelect = 0;
    menuHoldCounter = 0;
  }

  //choose a function based on the modeSelect value
  switch (modeSelect){
  case 0:
    menu();
    break;
  case 1:
    clockDisplay();
    break;
  case 2:
    stopWatch();
    break;
  }

  previousMillis = currentMillis;
}

void menu(){
  int menuSelect = 1; //variable for choosing a mode

  //indicate menu mode by turning on the button LEDs...
  digitalWrite(ledBGRN, true);
  digitalWrite(ledBRED, true);

  //indicate menu mode by flashing the display
  for(int x = 0; x < 3; x++){
    i2cSendValue(0); //Send all zeros to the display
    delay(250);
    clearDisplay();
    delay(250);
  }

  //loop until a mode other than menu is selected
  while(modeSelect == 0){
    i2cSendValue(menuSelect); //display the current mode selector value, starts at one
    delay(1); // a very small delay to prevent flickering

    //when the green button is pressed increment the menuSelect variable
    if (digitalRead(buttonGRN)){
      digitalWrite(ledBGRN, false);
      if (menuSelect <= 1){
        ++menuSelect;
      }
      else {
        menuSelect = 1;
      }
      delay(300);
      digitalWrite(ledBGRN, true);
    }

    //when the red button is pressed change the modeSelect, which will end the while loop and return to the main loop
    if (digitalRead(buttonRED)){
      digitalWrite(ledBRED, false);
      modeSelect = menuSelect;
      delay(500);
      digitalWrite(ledBGRN, false);
    }
  }
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write(0x77);  // Decimal control command
  Wire.write(0b00000010);  // Turns on the second decimal
  Wire.endTransmission();
  delay(100);
}

void stopWatch(){
  digitalWrite(ledBGRN, stopWatchRunning);
  digitalWrite(ledBRED, !stopWatchRunning);

  i2cSendValue((stopWatchMinute*100)+stopWatchSecond);  
  Serial.print("Stop Watch Running?: ");
  Serial.println(stopWatchRunning);
  Serial.print("Seconds: ");
  Serial.println(stopWatchSecond);
  Serial.print("Minutes: ");
  Serial.println(stopWatchMinute);
  Serial.print("Hours: ");
  Serial.println(stopWatchHour);

  getDate(); //retreive time from DS1037
  currentSecond = second;

  if(stopWatchRunning){
    getDate(); //retreive time from DS1037
    currentSecond = second;
    if (currentSecond != previousSecond){
      stopWatchSecond++;
      if (stopWatchSecond >= 60){
        stopWatchSecond = 0;
        stopWatchMinute++;
        if (stopWatchMinute >= 60){
          stopWatchMinute = 0;
          stopWatchHour++;
        }
      }
    }
  }

  if (digitalRead(buttonGRN) && stopWatchRunning == false){
    digitalWrite(ledBGRN, true);
    digitalWrite(ledBRED, false);
    stopWatchRunning = true;
    delay(500);
  }
  if (digitalRead(buttonRED) && stopWatchRunning == true){
    digitalWrite(ledBRED, true);
    digitalWrite(ledBGRN, false);
    stopWatchRunning = false;
    delay(500);
  }

  previousSecond = currentSecond;  
  delay(100);
}

byte bcdToDec(byte val){
  // Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

void getDate(){
  // Reset the register pointer
  Wire.beginTransmission(DS1307_ADDRESS);

  byte zero = 0x00;
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  second = bcdToDec(Wire.read());
  minute = bcdToDec(Wire.read());
  hour = bcdToDec(Wire.read() & 0b111111); //24 hour time
  int weekDay = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
  int monthDay = bcdToDec(Wire.read());
  int month = bcdToDec(Wire.read());
  int year = bcdToDec(Wire.read());

  //print the date EG   3/1/11 23:59:59
  /* Serial.print(month);
   Serial.print("/");
   Serial.print(monthDay);
   Serial.print("/");
   Serial.print(year);
   Serial.print(" ");
   Serial.print(hour);
   Serial.print(":");
   Serial.print(minute);
   Serial.print(":");
   Serial.println(second);*/
}

//Given a number, i2cSendValue chops up an integer into four values and sends them out over I2C
void i2cSendValue(int tempCycles){
  Wire.beginTransmission(DISPLAY_ADDRESS1); // transmit to device #1
  Wire.write(tempCycles / 1000); //Send the left most digit
  tempCycles %= 1000; //Now remove the left most digit from the number we want to display
  Wire.write(tempCycles / 100);
  tempCycles %= 100;
  Wire.write(tempCycles / 10);
  tempCycles %= 10;
  Wire.write(tempCycles); //Send the right most digit
  Wire.endTransmission(); //Stop I2C transmission
}

void clockDisplay(){
  getDate(); //retreive date and time from DS1307
  int currentTime = ((hour*100)+minute); //convert the time into a string to display on the sevSeg
  Serial.print("The Current Time is: ");
  Serial.println(currentTime);
  i2cSendValue(currentTime); //Send the four characters to the display
  delay(1000);
}

void clearDisplay(){
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write('v');
  Wire.endTransmission();
}














