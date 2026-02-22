/*
 * DS1307 RTC Quick Test — Arduino UNO Q
 *
 * Wiring:
 *   RTC SDA → A4
 *   RTC SCL → A5
 *   RTC VCC → 5V
 *   RTC GND → GND
 *   (CR2032 coin cell in RTC module for backup power)
 *
 * Libraries needed:
 *   - RTClib by Adafruit (Library Manager)
 *   - Arduino_RouterBridge (already installed)
 */

#include <Wire.h>
#include <RTClib.h>
#include <Arduino_RouterBridge.h>

RTC_DS1307 rtc;

const char daysOfWeek[7][4] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

void setup() {
  Monitor.begin(9600);
  delay(5000);  // wait for Serial Monitor to open — do NOT use while(!Monitor)
                // on Uno Q it hangs forever if Monitor isn't open yet

  Monitor.println("=== DS1307 RTC Test ===");
  delay(200);

  Wire.begin();
  delay(200);

  if (!rtc.begin()) {
    Monitor.println("ERROR: DS1307 not found on I2C bus!");
    delay(200);
    Monitor.println("Check wiring: SDA=A4, SCL=A5, VCC=5V, GND=GND");
    while (true) { delay(1000); }
  }

  Monitor.println("DS1307 found!");
  delay(200);

  if (!rtc.isrunning()) {
    Monitor.println("RTC not running — setting to compile time...");
    delay(200);
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Monitor.println("RTC time set.");
    delay(200);
  } else {
    Monitor.println("RTC already running.");
    delay(200);
  }

  Monitor.println("---");
  delay(200);
  printDateTime(rtc.now());
}

void loop() {
  printDateTime(rtc.now());
  delay(1000);
}

void printDateTime(const DateTime &dt) {
  char buf[40];
  snprintf(buf, sizeof(buf),
           "%04d/%02d/%02d (%s) %02d:%02d:%02d",
           dt.year(), dt.month(), dt.day(),
           daysOfWeek[dt.dayOfTheWeek()],
           dt.hour(), dt.minute(), dt.second());
  Monitor.println(buf);
  delay(200);
}
