/**
   Brady Parent
   LU ID: 0875611
   December 6, 2018
   This program is used to control the bTech Sump Pump Monitor (with temperature) controlled by Arduino Leonardo
*/
//include the required libraries
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include "DHT.h";
#include <NewPing.h>

//define the required variables for the hardware
#define BLYNK_PRINT Serial
#define TRIGGER_PIN  7
#define ECHO_PIN     8
#define MAX_DISTANCE 200
#define DHT11Pin 4
#define RGBPinG 10

// Hardware Serial on Leonardo
#define EspSerial Serial1

//ESP8266 baud rate:
#define ESP8266_BAUD 115200

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

//this is where you put the Blynk authorization token
//it can be found in the project settings under the nut icon
char auth[] = "BLYNK AUTH TOKEN HERE";

//wifi credentials go here
//wifi name goes in ssid. It must be typed EXACTLY as the network name is displayed
//the wifi password goes in pass. If it is an open network (no password), set this part as ""
//In the future, WiFi credentials will be stored in EEPROM from user input in Serial Monitor
char ssid[] = "NETWORK NAME HERE";
char pass[] = "PASSWORD HERE";
int waterNotification = 0;
int tempNotification = 0;
//In the future these values will be stored in EEPROM from user input in the Serial Monitor
float heightOverSump = 30; //in cm
float depthOfTank = 77; //in cm

ESP8266 wifi(&EspSerial);
BlynkTimer timer;
DHT dht(DHT11Pin, DHT11);

void setup()
{
  // Debug console
  Serial.begin(9600);

  // Set ESP8266 baud rate
  EspSerial.begin(ESP8266_BAUD);
  delay(10);

  //connect to Blynk through wifi
  Blynk.begin(auth, wifi, ssid, pass);

  //start the temp sensor
  dht.begin();

  pinMode(RGBPinG, OUTPUT);

  //calls checkTemperature every 10s and pushes it to the app
  //10s was chosen because temperature doesn't change very fast
  //and we are trying to avoid overloading the Blynk servers
  timer.setInterval(10000L, checkTemperature);

  //calls checkWaterLevel 4 times per second because water level can change
  //quickly and it's important to have an accurate reading within the app
  timer.setInterval(250L, checkWaterLevel);
} //end setup

void loop()
{

  if (Blynk.connected()) {
    Blynk.run();  // Initiates Blynk
    timer.run(); // Initiates BlynkTimer
    analogWrite(RGBPinG, 50); //Green light if connected to the internet
  } else {
    analogWrite(RGBPinG, 0);
  }

} //end loop

/**
   This function is used to calculate the current temperature in Celsius and
   push that information to the Blynk app
*/
void checkTemperature()
{
  //Read the temperature (c) value from the sensor.
  int tempCelsius = dht.readTemperature();

  //this section determines parameters to send notifications through the Blynk app
  if (tempCelsius > 35 && tempNotification == 0) {
    Blynk.notify("Temperature above 35°C. Check cooling system!");
    tempNotification = 1;
  } else if (tempCelsius < 12 && tempNotification == 0) {
    Blynk.notify("Temperature below 12°C. Check heating system!");
    tempNotification = 1;
  } else if (tempCelsius > 12 && tempCelsius < 35 && tempNotification > 0) {
    Blynk.notify("Temperature back within normal range");
    tempNotification = 0;
  }

  //Send temp to Blynk app via defined virtual pin
  Blynk.virtualWrite(V0, tempCelsius);

} //end checkTemperature

/**
   This function is used to determine the current sump water level
   and push that information to the user via the Blynk app
*/
void checkWaterLevel()
{
  //ping 25 times and collect median value, discarding any outliers
  int echoTime = sonar.ping_median(25);
  //converts echo time into distance as centimeters. Must cast as float since sonar only uses integer values
  float totalDistance = (float) sonar.convert_cm(echoTime);
  float waterDistance = totalDistance - heightOverSump;
  int waterLevelPercent;

  if (waterDistance < 0) {
    //set tank level to 100% if waterDistance is less than 0 (water is into the heightOverSump range)
    waterLevelPercent = 100;
  } else if (waterDistance > depthOfTank) {
    //set tank level to 0% if waterDistance is greater than depth of the tank
    waterLevelPercent = 0;
  } else {
    waterLevelPercent = (1 - (waterDistance / depthOfTank)) * 100;
  }

  //this section determines the parameters to send water level notifications through Blynk app
  if (waterLevelPercent > 50 && waterNotification == 0) {
    Blynk.notify("WATER LEVEL WARNING: Your sump is over 50% full!");
    waterNotification = 1;
  } else if (waterLevelPercent > 75 && waterNotification == 1) {
    Blynk.notify("WATER LEVEL WARNING: Your sump is over 75% full!");
    waterNotification = 2;
  } else if (waterLevelPercent > 90 && waterNotification == 2) {
    Blynk.notify("OVERFLOW WARNING: Your sump is over 90% full!");
    waterNotification = 3;
  } else if (waterLevelPercent > 99 && waterNotification == 3) {
    Blynk.notify("FLOOD WARNING: Your sump has overflown!");
    waterNotification = 4;
  } else if (waterLevelPercent <= 40 && waterNotification > 0) {
    Blynk.notify("Your sump water levels are back to normal");
    waterNotification = 0;
  }

  //Send water level to Blynk app via defined virtual pin
  Blynk.virtualWrite(V1, waterLevelPercent);
} //end checkWaterLevel
