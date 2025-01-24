/*
  @author Jacob JS
*/
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <NTPClient.h>    // https://github.com/arduino-libraries/NTPClient
#include <WiFiUdp.h>
#include <Wire.h>
#include <RTClib.h>       // https://github.com/adafruit/RTClib
#include <DMDESP.h>       // https://github.com/farid1991/DMDESP
#include <fonts/JAM11x28.h>

// WiFiManager Setup
WiFiManager wm;

// NTPClient Setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// RTC Setup
RTC_DS3231 rtc;

// DMDESP Setup
#define Font JAM11x28
#define DISPLAYS_WIDE 2 // Number of columns of P10 panels
#define DISPLAYS_HIGH 2 // Number of rows of P10 panels
DMDESP Disp(DISPLAYS_WIDE, DISPLAYS_HIGH);  // Initialize DMDESP

// Time Variables
int Jam = 0; // Hours
int Men = 0; // Minutes

void setup() {
  // Serial Init
  Serial.begin(115200);
  // NTPClient Init
  timeClient.begin();
  timeClient.setTimeOffset(3600 * 7); // Set to WIB (UTC+7)

  // Reset WiFi settings for testing (optional)
  wm.resetSettings();
  wm.setClass("invert");
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);

  if (wm.autoConnect("AP-JamKantin", "kantin1938")) {
    Serial.println("\nWiFi Connection Established!");
  } else {
    Serial.println("Config portal running.");
  }

  // RTC Init
  if (!rtc.begin()) {
    Serial.println("RTC not detected. Using NTP time only.");
    timeClient.update(); // Update time from NTP
    Jam = timeClient.getHours();
    Men = timeClient.getMinutes();
  } else {
    // Update RTC from NTP if WiFi is available
    if (WiFi.status() == WL_CONNECTED) {
      updateRTC();
    } else if (rtc.lostPower()) {
      Serial.println("RTC lost power and no WiFi available. Unable to update time.");
    }
  }

  // Initialize DMDESP
  Disp.start();
  Disp.setBrightness(100); // Brightness level
  Disp.setFont(Font);      // Set font
}

void updateRTC() {
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  DateTime ntpTime = DateTime(ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  rtc.adjust(ntpTime);
}

void loop() {
  wm.process();
  static unsigned long previousMillisBlink = 0;
  static unsigned long previousMillisRTC = 0;
  static bool showColon = true;

  if (WiFi.status() == WL_CONNECTED) {
    // Periodically update RTC from NTP
    if (millis() - previousMillisRTC > 600000) { // Update every 10 minutes
      Serial.println("Periodic RTC update from NTP...");
      updateRTC();
      previousMillisRTC = millis();
    }
  }

  if (rtc.begin()) {
    // Read time from RTC
    DateTime now = rtc.now();
    Jam = now.hour();
    Men = now.minute();
  } else {
    // If RTC not available, use NTP time
    Serial.println("RTC not detected. Using NTP time...");
    timeClient.update();
    Jam = timeClient.getHours();
    Men = timeClient.getMinutes();
  }

  // // Restart ESP if time is 00:00
  // if (Jam == 0 && Men == 0) {
  //   Serial.println("Time is 00:00. Restarting ESP...");
  //   ESP.restart();
  // }

  // Blink colon every 500 ms
  if (millis() - previousMillisBlink >= 500) {
    previousMillisBlink = millis();
    showColon = !showColon;
  }

  // Format time display
  char isi[6];
  if (showColon) {
    sprintf(isi, "%02d:%02d", Jam, Men); // With colon
  } else {
    sprintf(isi, "%02d!%02d", Jam, Men); // Without colon
  }
  
  // Display time on DMDESP
  Disp.setFont(Font);
  Disp.drawText(2, 2, isi); 
  Disp.loop();
}
