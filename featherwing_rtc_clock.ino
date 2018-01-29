/**
 * Simple RTC Digital Clock with Adafruit Feather/FeatherWings
 *
 * Feather / FeatherWings used
 *  - https://www.adafruit.com/product/2795 Feather 32u4 Adalogger
 *  - https://www.adafruit.com/product/3028 DS3231 Precision RTC FeatherWing
 *  - https://www.adafruit.com/product/3106 4-Digit 7-Segment FeatherWing
 *
 * Button (A0 pullup) for displaying text date on seven-segment display in
 * format: <DayOfWeek> <MonthAbbreviation> <Day> <Year> (Sun Jan 28 2018)
 * 
 * Credit: showTime function is based heavily on the work by Tony DiCola and 
 * Philip R. Moyer from Adafruit and should retain their MIT and BSD license
 * 
 * Segment alphabet values calculated using Jose Pino's excellent Excel template
 * http://www.josepino.com/microcontroller/7-segment-ascii 
 * 
 * @author Chris Fitzpatrick
 * @version 1.0 01/28/2018
 * @license MIT License 
 */

#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "RTClib.h"

#define DO_TEST       false // Run through a test displaying the text for Days and Months on start
#define TIME_24_HOUR  false // Use 24 Hour time
#define TIME_DELAY    1000  // Delay between poling RTC (1s)
#define TEXT_DELAY    3000  // Delay between displaying different text (3s)
#define BRIGHT_LOW    0     // Lowest Brightness
#define BRIGHT_HIGH   15    // Highest Brightness
#define PIN_DATE      A0    // Pin attached to button to display date

Adafruit_7segment matrix = Adafruit_7segment();
RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
int  matrixDays[7][3]  = {{109,28,84},    // Sun
                          {21,92,84},     // Mon
                          {7,28,123},     // Tue
                          {42,123,94},    // Wed
                          {7,116,28},     // Thu
                          {113,80,16},    // Fri
                          {109,95,120}};  // Sat
int matrixMonths[12][3] = {{14,95,84},    // Jan
                          {113,123,124},  // Feb
                          {21,95,80},     // Mar
                          {119,115,80},   // Apr
                          {21,95,110},    // May
                          {30,28,84},     // Jun
                          {30,28,48},     // Jul
                          {119,28,111},   // Aug
                          {109,123,115},  // Sep
                          {63,88,120},    // Oct
                          {55,92,28},     // Nov
                          {95,123,88}};   // Dec

bool blinkColon = false;

void setup() {
  // Pull up button pin
  pinMode(PIN_DATE, INPUT_PULLUP);
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // Boot the matrix
  matrix.begin(0x70);
  matrix.setBrightness(BRIGHT_LOW);

  // Display a greeting when we start
  if (DO_TEST) {
    showTest();
  } else {
    showGreeting();
  }
}

void loop() {
  // Get current time from RTC
  DateTime now = rtc.now();

  // If the button is pressed show the date, if not show the time
  if (digitalRead(PIN_DATE) == LOW) {
    showDate(now.dayOfTheWeek(), now.month(), now.day(), now.year());
  } else {
    showTime(now.hour(), now.minute());
  }

  // Display full time in Serial
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");  
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");
}

/**
 * ShowDate displays the date on the seven-segment display
 * 
 * Format is in Sun Jan 28 2018, each being displayed with a delay
 */
void showDate(int dayOfTheWeek, int month, int day, int year) {
  // Day Of Week
  showDayOfWeek(dayOfTheWeek);
  
  // Month
  showMonth(month);

  // Day
  matrix.print(day, DEC);
  matrix.writeDisplay();
  delay(TEXT_DELAY);

  // Year
  matrix.print(year, DEC);
  matrix.writeDisplay();
  delay(TEXT_DELAY);
}

/**
 * ShowTime displays the current time in 12 or 24hr format and will alternate
 * blinking of the colon.
 * 
 * Credit: This is based heavily on the work by Tony DiCola and Philip R. Moyer 
 * from Adafruit.
 * 
 * - https://learn.adafruit.com/7-segment-display-internet-clock/code (BSD)
 * - clock_sevenseg_ds1307 from clock_sevenseg_ds1307 from
 *   https://github.com/adafruit/Adafruit_LED_Backpack (MIT)
 *
 * This source code is in the public domain and is released under the MIT and BSD license.
 * Any further redistribution must include this header.
 *
 * @param int hour 
 * @param int minute
 */
void showTime(int hour, int minute) {
  // Build time for display
  int time = hour*100 + minute;
  if (!TIME_24_HOUR) {
    if (hour > 12) {
      time -= 1200;
    } else if (hour == 0) {
      time += 1200;
    }
  }
  matrix.print(time, DEC);
  
  // Add zero padding when in 24 hour mode and it's midnight.
  // In this case the print function above won't have leading 0's
  // which can look confusing.  Go in and explicitly add these zeros.
  if (TIME_24_HOUR && hour == 0) {
    // Pad hour 0.
    matrix.writeDigitNum(1, 0);
    // Also pad when the 10's minute is 0 and should be padded.
    if (minute < 10) {
      matrix.writeDigitNum(2, 0);
    }
  }
  
  // Alternate blinking of colon
  blinkColon = !blinkColon;
  matrix.drawColon(blinkColon);
  
  // Write to display and sleep for TIME_DELAY (1s)
  matrix.writeDisplay();
  delay(TIME_DELAY);
}

/**
 * ShowDayOfWeek displays short day abbreviation on a seven-segment display
 * 
 * @param int dayOfWeek the day of the week, 0 (Sun) -> 6 (Sat)
 */
void showDayOfWeek(int dayOfTheWeek) {
  matrix.clear();
  matrix.writeDigitRaw(1,matrixDays[dayOfTheWeek][0]);
  matrix.writeDigitRaw(3,matrixDays[dayOfTheWeek][1]);
  matrix.writeDigitRaw(4,matrixDays[dayOfTheWeek][2]);
  matrix.writeDisplay();
  delay(TEXT_DELAY);
}

/**
 * ShowMonth displays short month abbreviation on seven-segment display
 * 
 * @param int month the month, 1 (Jan) -> 12 (Dec)
 */
void showMonth(int month) {
  month -= 1; // Offset for RTCLib Month which starts 1 = Jan vs regular DateTime where 0 is Jan
  matrix.clear();
  matrix.writeDigitRaw(1,matrixMonths[month][0]);
  matrix.writeDigitRaw(3,matrixMonths[month][1]);
  matrix.writeDigitRaw(4,matrixMonths[month][2]);
  matrix.writeDisplay();
  delay(TEXT_DELAY);   
}

/**
 * ShowGreeting displays a short greeting on start
 */
void showGreeting() {
  matrix.clear();
  matrix.writeDigitRaw(0,116);  //h
  matrix.writeDigitRaw(1,121);  //E
  matrix.writeDigitRaw(3,56);   //L
  matrix.writeDigitRaw(4,92);   //o
  matrix.writeDisplay();
  delay(TEXT_DELAY);

  matrix.clear();
  matrix.writeDigitRaw(0,42); //W
  matrix.writeDigitRaw(1,80); //r
  matrix.writeDigitRaw(3,56); //L
  matrix.writeDigitRaw(4,94); //d
  matrix.writeDisplay();
  delay(TEXT_DELAY);    
}

/**
 * ShowTest displays all dayOfTheWeek and month abbreviation texts
 */
void showTest() {
  delay(1000);
  Serial.println("Test Week");
  for (int i=0; i<7; i++) {
    Serial.println(i);
    showDayOfWeek(i);
  }
  Serial.println("Test Month");
  for (int i=1; i<=12; i++) {
    Serial.println(i);
    showMonth(i);
  }
}

