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
 * Open Serial Monitor at 115200 baud to see output.
 */

#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 rtc;

const char daysOfWeek[7][4] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }          // wait for Serial on native-USB boards

  Wire.begin();
  delay(100);                     // let I2C settle

  Serial.println(F("=== DS1307 RTC Test ==="));

  if (!rtc.begin()) {
    Serial.println(F("ERROR: Could not find DS1307 on I2C bus!"));
    Serial.println(F("  -> Check wiring: SDA=A4, SCL=A5, VCC=5V"));
    while (true) { delay(1000); } // halt
  }

  Serial.println(F("DS1307 found on I2C bus."));

  if (!rtc.isrunning()) {
    Serial.println(F("RTC is NOT running — setting time to compile timestamp..."));
    // This sets the RTC to the date & time this sketch was compiled.
    // Upload again if you need to re-sync.
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(F("RTC time set."));
  } else {
    Serial.println(F("RTC is already running."));
  }

  // Print the current time once immediately
  printDateTime(rtc.now());
  Serial.println(F("---"));
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

  Serial.println(buf);
}
