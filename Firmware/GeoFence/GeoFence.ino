/*
  GeoFence Board Firmware
  By: Nick Poole
  SparkFun Electronics
  Date: July 10th, 2017
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  GeoFence has an ATmega running at 8MHz/3.3V. Select 'Pro Mini @ 3.3V/8MHz' from Boards menu to program
  the device.
*/

#include <SparkFun_I2C_GPS_Arduino_Library.h> //Use Library Manager or download here: https://github.com/sparkfun/SparkFun_I2C_GPS_Arduino_Library
I2CGPS myI2CGPS; //Hook object to the library

#include <TinyGPS++.h> //From: https://github.com/mikalhart/TinyGPSPlus
TinyGPSPlus gps; //Declare gps object

#include <EEPROM.h>

double zone1[5]; //Contains the 5 GPS coordinates for each zone
double zone2[5];
double zone3[5];
double zone4[5];
char zoneType[4];

byte STAT = 6; //Blue status LED next to ATmega
byte zoneIOPin[] = {7, 8, 9, 10}; //Four zone LEDs

#define MODE_WAITING_FOR_LOCK 0
#define MODE_GPS_LOCKED 1

byte systemMode; //Keeps track of what mode we are in so as to control the status LED

void setup()
{
  //Setup I/O
  pinMode(STAT, OUTPUT);
  digitalWrite(STAT, HIGH); //Indicate we are waiting for GPS data

  loadConfig();

  //Configure Zone LEDs
  for (byte z = 0; z < 4; z++)
  {
    pinMode(zoneIOPin[z], OUTPUT);
    digitalWrite(zoneIOPin[z], LOW);
  }

  //ATmega is running at 8MHz(3.3V). Serial above 57600bps is not good.
  Serial.begin(9600);

  if (myI2CGPS.begin() == false)
  {
    //GPS communication failure
    Serial.println("GPS module failed to respond. Please contact support.");

    //Blink STAT and Zone LEDs to indicate fault
    while (1) //Freeze!
    {
      digitalWrite(zoneIOPin[0], HIGH);
      digitalWrite(zoneIOPin[1], LOW);
      digitalWrite(zoneIOPin[2], HIGH);
      digitalWrite(zoneIOPin[3], LOW);
      digitalWrite(STAT, HIGH);
      delay(1000);
      digitalWrite(zoneIOPin[0], LOW);
      digitalWrite(zoneIOPin[1], HIGH);
      digitalWrite(zoneIOPin[2], LOW);
      digitalWrite(zoneIOPin[3], HIGH);
      digitalWrite(STAT, LOW);
      delay(1000);
    }
  }

  systemMode = MODE_WAITING_FOR_LOCK;
}

void loop()
{
  // Do GeoFence Things
  while (myI2CGPS.available()) //available() returns the number of new bytes available from the GPS module
  {
    gps.encode(myI2CGPS.read()); //Feed the GPS parser
  }

  if (gps.time.isUpdated() && gps.location.isValid()) //Check to see if new GPS info is available
  {
    Serial.println("Valid Data -> Updating");
    updateGeofence();
  }

  //Once we have lock, go to new STAT LED mode
  if (gps.location.isValid())
    systemMode = MODE_GPS_LOCKED;
  else
    systemMode = MODE_WAITING_FOR_LOCK;


  // Check for a new config
  if (Serial.available()) {
    while (Serial.read() != '$') {};
    String configString = Serial.readStringUntil('$');
    configure(configString);
  }

  //Control the status LED
  if (systemMode == MODE_WAITING_FOR_LOCK)
  {
    //Blink LED every second
    int onSeconds = millis() / 1000;
    if (onSeconds % 2 == 0)
      digitalWrite(STAT, HIGH);
    else
      digitalWrite(STAT, LOW);
  }
  else if (systemMode == MODE_GPS_LOCKED)
  {
    //Do LED heart beat
    int onSeconds = millis() / 1000;
    if (onSeconds % 4 == 0)
    {
      //Every 4th second, do a heart beat
      int ledValue = millis() % 1000; //6721 % 1000 = 721

      if (ledValue <= 500)
      {
        //Bring LED up in brightness
        //Take the current time and turn it into a value 0 to 255
        ledValue = map(ledValue, 0, 500, 0, 255);
      }
      else
      {
        //Bring LED down in brightness
        //Take the current time and turn it into a value 0 to 255
        ledValue = map(ledValue, 500, 1000, 255, 0); //721 becomes 113
      }
      analogWrite(STAT, ledValue);
    }
    else
    {
      //Leave LED off for a second
      digitalWrite(STAT, LOW);
    }
  } //End STAT LED control


}

void configure(String configString)
{
  //Clear current EEPROM settings
  for (byte i = 0 ; i < 100 ; i++)
    EEPROM.write(i, 0);

  // Receive and Verify a configuration
  String linebuffer;
  int addr = 0;
  int strIndex = 0;
  int inByte = 0;
  boolean checksumPass = 0;
  int attempts = 0;

  while (!checksumPass && attempts < 5) {

    attempts++;

    digitalWrite(STAT, LOW);

    strIndex++;

    //Serial.println("parsing...");

    for (int zone = 0; zone < 4; zone++) {
      inByte = configString.charAt(strIndex); // Should contain Zone 1 Type (R/C/X)
      strIndex++; strIndex++;

      switch (inByte) {

        case 'R': //Zone is rectangular

          addr = 0 + (zone * 20);
          linebuffer.remove(0);
          while (configString.charAt(strIndex) != 10) {
            linebuffer += configString.charAt(strIndex);
            strIndex++;
          }
          strIndex++;
          str_to_double_to_EEPROM(linebuffer, addr);

          addr = 4 + (zone * 20);
          linebuffer.remove(0);
          while (configString.charAt(strIndex) != 10) {
            linebuffer += configString.charAt(strIndex);
            strIndex++;
          }
          strIndex++;
          str_to_double_to_EEPROM(linebuffer, addr);

          addr = 8 + (zone * 20);
          linebuffer.remove(0);
          while (configString.charAt(strIndex) != 10) {
            linebuffer += configString.charAt(strIndex);
            strIndex++;
          }
          strIndex++;
          str_to_double_to_EEPROM(linebuffer, addr);

          addr = 12 + (zone * 20);
          linebuffer.remove(0);
          while (configString.charAt(strIndex) != 10) {
            linebuffer += configString.charAt(strIndex);
            strIndex++;
          }
          strIndex++;
          str_to_double_to_EEPROM(linebuffer, addr);

          break;

        case 'C': //Zone is circular

          addr = 8 + (zone * 20);
          linebuffer.remove(0);
          while (configString.charAt(strIndex) != 10) {
            linebuffer += configString.charAt(strIndex);
            strIndex++;
          }
          strIndex++;
          str_to_double_to_EEPROM(linebuffer, addr);

          addr = 12 + (zone * 20);
          linebuffer.remove(0);
          while (configString.charAt(strIndex) != 10) {
            linebuffer += configString.charAt(strIndex);
            strIndex++;
          }
          strIndex++;
          str_to_double_to_EEPROM(linebuffer, addr);

          addr = 16 + (zone * 20);
          linebuffer.remove(0);
          while (configString.charAt(strIndex) != 10) {
            linebuffer += configString.charAt(strIndex);
            strIndex++;
          }
          strIndex++;
          str_to_double_to_EEPROM(linebuffer, addr);

          break;

        case 'X': //Zone is undefined
          break;

      }
    }

    //Serial.println("parsed");

    strIndex = strIndex + 2;
    linebuffer.remove(0);
    while (configString.charAt(strIndex) != 10) {
      linebuffer += configString.charAt(strIndex);
      strIndex++;
    }

    //Serial.print("checksum received: ");
    //Serial.println(linebuffer.toInt());

    int calcSum = 0;

    //Serial.println("ready to calculate sum");

    for (int r = 0; configString.charAt(r) != '^'; r++) {
      calcSum = calcSum + configString.charAt(r);
    }

    calcSum = calcSum + 140;
    calcSum = calcSum % 256;

    //Serial.print("checksum calculated: ");
    //Serial.println(calcSum);

    if (linebuffer.toInt() == calcSum)
    {
      for (byte x = 0 ; x < 5 ; x++)
      {
        digitalWrite(STAT, HIGH);
        delay(200);
        digitalWrite(STAT, LOW);
        delay(200);
      }

      Serial.print("$\n"); //Checksum passed
      checksumPass = 1;
    } else {
      Serial.print("!\n"); //Checksum failed
      linebuffer.remove(0);
      addr = 0;
      strIndex = 0;
      inByte = 0;
      delay(500);
    }
  }
}

void loadConfig() {

  // Pull config from EEPROM

  for (int i = 0; i < 5; i++) {
    EEPROM.get(i * 4, zone1[i]);
  }
  for (int i = 0; i < 5; i++) {
    EEPROM.get(20 + (i * 4), zone2[i]);
  }
  for (int i = 0; i < 5; i++) {
    EEPROM.get(40 + (i * 4), zone3[i]);
  }
  for (int i = 0; i < 5; i++) {
    EEPROM.get(60 + (i * 4), zone4[i]);
  }

  if (zone1[0] == 0 && zone1[2] == 0) {
    zoneType[0] = 'X'; //Not yet filled with coordinates
  }
  if (zone1[0] == 0 && zone1[2] != 0) {
    zoneType[0] = 'C'; //Circle
  }
  if (zone1[0] != 0) {
    zoneType[0] = 'R'; //Rectangle
  }

  if (zone2[0] == 0 && zone2[2] == 0) {
    zoneType[1] = 'X';
  }
  if (zone2[0] == 0 && zone2[2] != 0) {
    zoneType[1] = 'C';
  }
  if (zone2[0] != 0) {
    zoneType[1] = 'R';
  }

  if (zone3[0] == 0 && zone3[2] == 0) {
    zoneType[2] = 'X';
  }
  if (zone3[0] == 0 && zone3[2] != 0) {
    zoneType[2] = 'C';
  }
  if (zone3[0] != 0) {
    zoneType[2] = 'R';
  }

  if (zone4[0] == 0 && zone4[2] == 0) {
    zoneType[3] = 'X';
  }
  if (zone4[0] == 0 && zone4[2] != 0) {
    zoneType[3] = 'C';
  }
  if (zone4[0] != 0) {
    zoneType[3] = 'R';
  }
}

void str_to_double_to_EEPROM(String str, int addr) {
  double bytes = str.toDouble();
  EEPROM.put(addr, bytes);
}

void updateGeofence() {

  for (int zone = 0; zone < 4; zone++) {

    switch (zoneType[zone]) {

      case 'R':
        Serial.print("Checking Rectangular Zone ");
        Serial.println(zone);
        digitalWrite(zoneIOPin[zone], checkRectangle(zone));
        break;

      case 'C':
        Serial.print("Checking Circular Zone ");
        Serial.println(zone);
        digitalWrite(zoneIOPin[zone], checkCircle(zone));
        break;

    }
  }
}

boolean checkRectangle(int zone) {

  double swLat, swLng, neLat, neLng;

  switch (zone) {

    case 0:
      neLat = zone1[0];
      neLng = zone1[1];
      swLat = zone1[2];
      swLng = zone1[3];
      break;

    case 1:
      neLat = zone2[0];
      neLng = zone2[1];
      swLat = zone2[2];
      swLng = zone2[3];
      break;

    case 2:
      neLat = zone3[0];
      neLng = zone3[1];
      swLat = zone3[2];
      swLng = zone3[3];
      break;

    case 3:
      neLat = zone4[0];
      neLng = zone4[1];
      swLat = zone4[2];
      swLng = zone4[3];
      break;
  }

  if (neLng < swLng) { //If this is true, we're stradling the 180th. This is bad for the maths.

    //We must slice the box into two boxes with the max of one being 180 and the min of the other being -180

    boolean zoneSubA = (min(neLng, swLng) > gps.location.lng() && gps.location.lng() > -180 && min(neLat, swLat) < gps.location.lat() && gps.location.lat() < max(neLat, swLat));
    boolean zoneSubB = (max(neLng, swLng) < gps.location.lng() && gps.location.lng() < 180 && min(neLat, swLat) < gps.location.lat() && gps.location.lat() < max(neLat, swLat));

    //If the current location falls into either of these halves, we're in the zone

    if (zoneSubA || zoneSubB) {
      return 1;
    } else {
      return 0; //If not, return 0
    }

  } else { //In a more reasonable locale

    //Nothing too fancy here, if our point is between the top and bottom of the rectangle AND between the sides... we're in!

    Serial.println("Performing Box Test");

    Serial.print("Min(x1,x2): ");
    Serial.println(min(neLat, swLat), 6);
    Serial.print("Lat: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Max(x1,x2): ");
    Serial.println(max(neLat, swLat), 6);
    Serial.print("Min(y1,y2): ");
    Serial.println(min(neLng, swLng), 6);
    Serial.print("Lng: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Max(y1,y2): ");
    Serial.println(max(neLng, swLng), 6);

    if (min(neLat, swLat) < gps.location.lat() && gps.location.lat() < max(neLat, swLat) && min(neLng, swLng) < gps.location.lng() && gps.location.lng() < max(neLng, swLng)) {
      Serial.println("Box Test Passed");
      return 1;
    } else {
      Serial.println("Box Text Failed");
      return 0; //If not, return 0
    }
  }
}

boolean checkCircle(int zone) {

  double centerLat, centerLng, zoneRadius;

  switch (zone) {

    case 0:
      centerLat = zone1[2];
      centerLng = zone1[3];
      zoneRadius = zone1[4];
      break;

    case 1:
      centerLat = zone2[2];
      centerLng = zone2[3];
      zoneRadius = zone2[4];
      break;

    case 2:
      centerLat = zone3[2];
      centerLng = zone3[3];
      zoneRadius = zone3[4];
      break;

    case 3:
      centerLat = zone4[2];
      centerLng = zone4[3];
      zoneRadius = zone4[4];
      break;
  }

  double distanceToCenter =
    TinyGPSPlus::distanceBetween(
      gps.location.lat(),
      gps.location.lng(),
      centerLat,
      centerLng);

  Serial.println("Performing Circle Test");
  Serial.print("Distance To: ");
  Serial.println(distanceToCenter);
  Serial.print("Radius: ");
  Serial.println(zoneRadius);

  if (distanceToCenter < zoneRadius) {
    Serial.println("Circle Test Passed");
    return 1;
  } else {
    Serial.println("Circle Test Failed");
    return 0;
  }
}

