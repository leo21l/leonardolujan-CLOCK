
//include libraries:
#include "LedControl.h"
#include <FontLEDClock.h>                // Font library
#include <Wire.h>                        // DS1307 clock
#include "RTClib.h"                      // DS1307 clock
#include <Button.h>                      // Button library by Alexander Brevig
#include "Adafruit_ADT7410.h"            // ADT714O temp sensor

// Setup LED Matrix

// pin 12 is connected to the DataIn on the display
// pin 11 is connected to the CLK on the display
// pin 10 is connected to LOAD on the display
#define PIN_LED_CONTROLLER_DATA   10
#define PIN_LED_CONTROLLER_CLK    9
#define PIN_LED_CONTROLLER_LOAD   8

//LedControl lc=LedControl(12,11,10,1);
LedControl lc = LedControl(PIN_LED_CONTROLLER_DATA, PIN_LED_CONTROLLER_CLK,
                                PIN_LED_CONTROLLER_LOAD, 4);

//global variables
byte intensity = 5;                      // Default intensity/brightness (0-15)
byte clock_mode = 0;                     // Default clock mode. Default = 0 (basic_mode)
bool random_mode = 0;                    // Define random mode - changes the display type every few hours. Default = 0 (off)
byte old_mode = clock_mode;              // Stores the previous clock mode, so if we go to date or whatever, we know what mode to go back to after.
bool ampm = 1;                           // Define 12 or 24 hour time. 0 = 24 hour. 1 = 12 hour
byte change_mode_time = 0;               // Holds hour when clock mode will next change if in random mode.
unsigned long delaytime = 500;           // We always wait a bit between updates of the display
int rtc[7];                              // Holds real time clock output



char days[7][4] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
}; //day array - used in slide, basic_mode and jumble modes (The DS1307 outputs 1-7 values for day of week)
char daysfull[7][9] = {
  "Sunday", "Monday", "Tuesday", "Wed", "Thursday", "Friday", "Saturday"
};
char suffix[4][3] = {
  "st", "nd", "rd", "th"
};  //date suffix array, used in slide, basic_mode and jumble modes. e,g, 1st 2nd ...

//define constants
#define NUM_DISPLAY_MODES 3              // Number display modes (conting zero as the first mode)
#define NUM_SETTINGS_MODES 4             // Number settings modes = 6 (conting zero as the first mode)
#define SLIDE_DELAY 20                   // The time in milliseconds for the slide effect per character in slide mode. Make this higher for a slower effect
#define cls          clear_display       // Clear display

RTC_DS1307 ds1307;                              // Create RTC object
Adafruit_ADT7410 tempsensor = Adafruit_ADT7410(); //Create ADT7410 object

Button buttonA = Button(5, BUTTON_PULLUP);      // Setup button A (using button library) MODE
Button buttonB = Button(4, BUTTON_PULLUP);      // Setup button B (using button library) UP
Button buttonC = Button(2, BUTTON_PULLUP);      // Setup button C (using button library) DOWN
Button buttonD = Button(3, BUTTON_PULLUP);      // Setup button D (using button library) SELECT

void setup() {

  digitalWrite(2, HIGH);                 // turn on pullup resistor for button on pin 2
  digitalWrite(3, HIGH);                 // turn on pullup resistor for button on pin 3
  digitalWrite(4, HIGH);                 // turn on pullup resistor for button on pin 4
  digitalWrite(5, HIGH);                 // turn on pullup resistor for button on pin 5 
  Serial.begin(9600); //start serial

  //initialize the 4 matrix panels
  //we have already set the number of devices when we created the LedControl
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  for (int address = 0; address < devices; address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(address, false);
    /* Set the brightness to a medium values */
    lc.setIntensity(address, intensity);
    /* and clear the display */
    lc.clearDisplay(address);
  }

  //Setup DS1307 RTC
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino
#endif
  ds1307.begin(); //start RTC Clock

  if (! ds1307.isrunning()) {
    Serial.println("RTC is NOT running!");
    ds1307.adjust(DateTime(__DATE__, __TIME__));  // sets the RTC to the date & time this sketch was compiled
  }
  //Show software version & hello message
  printver();
  
  //enable red led
  digitalWrite(13, HIGH);
} 

  //Setup ADT7410 TempSensor
#ifdef AVR
  Wire.begin();
#else
  Wire2.begin(); // Shield I2C pins connect to alt I2C bus on Arduino
#endif
  adt7410.begin(); //start temp sensor
  
  // Make sure the sensor is found, you can also pass in a different i2c
  // address with tempsensor.begin(0x49) for example
  if (!tempsensor.isrunning()) {
    Serial.println("Couldn't find ADT7410!");
 
   }
  //Show software version & hello message
  printver();
  
  //enable red led
  digitalWrite(13, HIGH);


void loop() {
  
  //run the clock with whatever mode is set by clock_mode - the default is set at top of code.
  switch (clock_mode){
        
  case 0: 
    basic_mode();
    break; 
   case 1:
    temp_mode();
    break;
  }
}

//plot a point on the display
void plot (byte x, byte y, byte val) {

  //select which matrix depending on the x coord
  byte address;
  if (x >= 0 && x <= 7)   {
    address = 3;
  }
  if (x >= 8 && x <= 15)  {
    address = 2;
    x = x - 8;
  }
  if (x >= 16 && x <= 23) {
    address = 1;
    x = x - 16;
  }
  if (x >= 24 && x <= 31) {
    address = 0;
    x = x - 24;
  }

  if (val == 1) {
    lc.setLed(address, y, x, true);
  } else {
    lc.setLed(address, y, x, false);
  }
}


//clear screen
void clear_display() {
  for (byte address = 0; address < 4; address++) {
    lc.clearDisplay(address);
  }
}

//fade screen down
void fade_down() {

  //fade from global intensity to 1
  for (byte i = intensity; i > 0; i--) {
    for (byte address = 0; address < 4; address++) {
      lc.setIntensity(address, i);
    }
    delay(50); //change this to change fade down speed
  }

  clear_display(); //clear display completely (off)

  //reset intentsity to global val
  for (byte address = 0; address < 4; address++) {
    lc.setIntensity(address, intensity);
  }
}



//power up led test & display software version number
void printver() {

  byte i = 0;
  char ver_a[9] = " HELLO! ";
  char ver_b[9] = "LEONARDO";

  //test all leds.
  for (byte x = 0; x <= 31; x++) {
    for (byte y = 0; y <= 7; y++) {
      plot(x, y, 1);
    }
  }
  delay(500);
  fade_down();

  while (ver_a[i]) {
    puttinychar((i * 4), 1, ver_a[i]);
    delay(35);
    i++;
  }
  delay(700);
  fade_down();
  i = 0;
  while (ver_b[i]) {
    puttinychar((i * 4), 1, ver_b[i]);
    delay(35);
    i++;
  }
  delay(700);
  fade_down();
}


// puttinychar
// Copy a 3x5 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
// This is unoptimized and simply uses plot() to draw each dot.
void puttinychar(byte x, byte y, char c)
{
  byte dots;
  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
    c &= 0x1F;   // A-Z maps to 1-26
  }
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 32;
  }
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 27; // full stop
  }
  else if (c == ':') {
    c = 28; // colon
  }
  else if (c == '\'') {
    c = 29; // single quote mark
  }
  else if (c == '!') {
    c = 30; // single quote mark
  }
  else if (c == '?') {
    c = 31; // single quote mark
  }

  for (byte col = 0; col < 3; col++) {
    dots = pgm_read_byte_near(&mytinyfont[c][col]);
    for (char row = 0; row < 5; row++) {
      if (dots & (16 >> row))
        plot(x + col, y + row, 1);
      else
        plot(x + col, y + row, 0);
    }
  }
}



void putnormalchar(byte x, byte y, char c)
{

  byte dots;
  //  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
  //    c &= 0x1F;   // A-Z maps to 1-26
  //  }
  if (c >= 'A' && c <= 'Z' ) {
    c &= 0x1F;   // A-Z maps to 1-26
  }
  else if (c >= 'a' && c <= 'z') {
    c = (c - 'a') + 41;   // A-Z maps to 41-67
  }
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 31;
  }
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 27; // full stop
  }
  else if (c == '\'') {
    c = 28; // single quote mark
  }
  else if (c == ':') {
    c = 29; // clock_mode selector arrow
  }
  else if (c == '>') {
    c = 30; // clock_mode selector arrow
  }
  else if (c >= -80 && c <= -67) {
    c *= -1;
  }

  for (char col = 0; col < 5; col++) {
    dots = pgm_read_byte_near(&myfont[c][col]);
    for (char row = 0; row < 7; row++) {
      //check coords are on screen before trying to plot
      //if ((x >= 0) && (x <= 31) && (y >= 0) && (y <= 7)){

      if (dots & (64 >> row)) {   // only 7 rows.
        plot(x + col, y + row, 1);
      } else {
        plot(x + col, y + row, 0);
      }
      //}
    }
  }
}

// basic_mode()
// show the time in 5x7 characters
void basic_mode()
{
  cls();

  char buffer[3];   //for int to char conversion to turn rtc values into chars we can print on screen
  byte offset = 0;  //used to offset the x postition of the digits and centre the display when we are in 12 hour mode and the clock shows only 3 digits. e.g. 3:21
  byte x, y;        //used to draw a clear box over the left hand "1" of the display when we roll from 12:59 -> 1:00am in 12 hour mode.

  //do 12/24 hour conversion if ampm set to 1
  byte hours = rtc[2];

  if (hours > 12) {
    hours = hours - ampm * 12;
  }
  if (hours < 1) {
    hours = hours + ampm * 12;
  }

  //do offset conversion
  if (ampm && hours < 10) {
    offset = 2;
  }
  
  //set the next minute we show the date at
  //set_next_date();
  
  // initially set mins to value 100 - so it wll never equal rtc[1] on the first loop of the clock, meaning we draw the clock display when we enter the function
  byte secs = 100;
  byte mins = 100;
  int count = 0;
  
  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    //get the time from the clock chip
    get_time();
    
    //check for button press
    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
    if (buttonB.uniquePress()) {
      display_date();
      return;
    }

    //check whether it's time to automatically display the date
    //check_show_date();

    //draw the flashing : as on if the secs have changed.
    if (secs != rtc[0]) {

      //update secs with new value
      secs = rtc[0];

      //draw :
      plot (15 - offset, 2, 1); //top point
      plot (15 - offset, 5, 1); //bottom point
      count = 400;
    }

    //if count has run out, turn off the :
    if (count == 0) {
      plot (15 - offset, 2, 0); //top point
      plot (15 - offset, 5, 0); //bottom point
    }
    else {
      count--;
    }

    //re draw the display if button pressed or if mins != rtc[1] i.e. if the time has changed from what we had stored in mins, (also trigggered on first entering function when mins is 100)
    if (mins != rtc[1]) {

      //update mins and hours with the new values
      mins = rtc[1];
      hours = rtc[2];

      //adjust hours of ampm set to 12 hour mode
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      itoa(hours, buffer, 10);

      //if hours < 10 the num e.g. "3" hours, itoa coverts this to chars with space "3 " which we dont want
      if (hours < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      //print hours
      //if we in 12 hour mode and hours < 10, then don't print the leading zero, and set the offset so we centre the display with 3 digits.
      if (ampm && hours < 10) {
        offset = 2;

        //if the time is 1:00am clear the entire display as the offset changes at this time and we need to blank out the old 12:59
        if ((hours == 1 && mins == 0) ) {
          cls();
        }
      }
      else {
        //else no offset and print hours tens digit
        offset = 0;

        //if the time is 10:00am clear the entire display as the offset changes at this time and we need to blank out the old 9:59
        if (hours == 10 && mins == 0) {
          cls();
        }


        putnormalchar(1,  0, buffer[0]);
      }
      //print hours ones digit
      putnormalchar(7 - offset, 0, buffer[1]);


      //print mins
      //add leading zero if mins < 10
      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //print mins tens and ones digits
      putnormalchar(19 - offset, 0, buffer[0]);
      putnormalchar(25 - offset, 0, buffer[1]);
    }
  }
  fade_down();
}

  //TempMode
  // Show temp in C and F

void temp_mode()
{
  cls();

  char
}
