/*
 * DS1307 RTC Quick Test
 * ---------------------
 * Wiring:  SDA → A4,  SCL → A5,  VCC → 5 V,  GND → GND
 *          (CR2032 coin cell in the RTC module for backup)
 *
 * What this does:
 *   1. Tries to talk to the DS1307 over I2C.
 *   2. If the RTC lost power (or was never set), programs it with
 *      the compile-time timestamp so the clock starts ticking.
 *   3. Prints date + time to Serial every second.
 *
 * Open the Arduino Cloud Monitor to see output.
 */

#include <Wire.h>
#include <RTClib.h>
#include <Arduino_UnifiedStorage.h>

RTC_DS1307 rtc;

const char daysOfWeek[7][4] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

void setup() {
  Monitor.begin(9600);
  while (!Monitor) { ; }         // wait for Monitor connection

  Wire.begin();
  delay(100);                     // let I2C settle

  Monitor.println("=== DS1307 RTC Test ===");

  if (!rtc.begin()) {
    delay(100);
    Monitor.println("ERROR: Could not find DS1307 on I2C bus!");
    delay(100);
    Monitor.println("  -> Check wiring: SDA=A4, SCL=A5, VCC=5V");
    while (true) { delay(1000); } // halt
  }

  Monitor.println("DS1307 found on I2C bus.");

  if (!rtc.isrunning()) {
    delay(100);
    Monitor.println("RTC is NOT running — setting time to compile timestamp...");
    // This sets the RTC to the date & time this sketch was compiled.
    // Upload again if you need to re-sync.
    delay(100);
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Monitor.println("RTC time set.");
  } else {
    delay(100);
    Monitor.println("RTC is already running.");
  }

  // Print the current time once immediately
  printDateTime(rtc.now());
  delay(100);
  Monitor.println("---");
}

void loop() {
  DateTime now = rtc.now();
  printDateTime(now);
  delay(1000);
}

void printDateTime(const DateTime &dt) {
  char buf[40];

  // YYYY/MM/DD (DDD) HH:MM:SS
  snprintf(buf, sizeof(buf),
           "%04d/%02d/%02d (%s) %02d:%02d:%02d",
           dt.year(), dt.month(), dt.day(),
           daysOfWeek[dt.dayOfTheWeek()],
           dt.hour(), dt.minute(), dt.second());
  delay(100);
  Monitor.println(buf);
}
