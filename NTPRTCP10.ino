/*
 @author Jacob JS
*/

#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <RTClib.h>
#include <DMDESP.h>
#include <fonts/JAM15x30.h>

const char *ssid     = "TongTji";
const char *password = "1938@tongtji";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
RTC_DS3231 rtc;
// DMDESP Setup
// #define Font EMSansSP8x16
#define Font JAM15x30
#define DISPLAYS_WIDE 2 // Kolom Panel
#define DISPLAYS_HIGH 2 // Baris Panel
DMDESP Disp(DISPLAYS_WIDE, DISPLAYS_HIGH);  // Jumlah Panel P10 yang digunakan (KOLOM,BARIS)
byte Jam;
byte Men;

void setup() {
  // Serial Init
  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");

  // NTPClient Init
  timeClient.begin();
  timeClient.setTimeOffset(3600 * 7); // Waktu Indonesia Barat

  // RTC Init
  if (!rtc.begin()) {
    Serial.println("RTC not detected. Using NTP time only.");
    timeClient.update(); // Update waktu dari NTP
    Jam = timeClient.getHours();
    Men = timeClient.getMinutes();
  } else {
    // Update RTC from NTP if Wi-Fi is available
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Updating RTC from NTP...");
      setRTCFromNTP(true);
    } else if (rtc.lostPower()) {
      Serial.println("RTC lost power and no Wi-Fi available. Unable to update time.");
    }
  }

  // Initialize DMDESP
  Disp.start();
  Disp.setBrightness(100); // Tingkat kecerahan
  Disp.setFont(Font);      // Huruf
}

void setRTCFromNTP(bool updateRTC) {
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  DateTime ntpTime = DateTime(ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

  // Print NTP time
  Serial.print("NTP Time: ");
  Serial.print(ntpTime.year());
  Serial.print("-");
  Serial.print(ntpTime.month());
  Serial.print("-");
  Serial.print(ntpTime.day());
  Serial.print(" ");
  Serial.print(ntpTime.hour());
  Serial.print(":");
  Serial.print(ntpTime.minute());
  Serial.print(":");
  Serial.println(ntpTime.second());

  Jam = ntpTime.hour();
  Men = ntpTime.minute();

  // Check RTC vs NTP time difference
  if (updateRTC) {
    DateTime rtcTime = rtc.now();
    long diff = (long)rtcTime.unixtime() - (long)ntpTime.unixtime();
    diff = diff < 0 ? -diff : diff; // Absolute value

    if (diff > 5) { // If time difference exceeds 5 seconds
      Serial.println("RTC time difference too large. Updating RTC from NTP...");
      rtc.adjust(ntpTime);
      Serial.println("RTC updated successfully.");
    } else {
      Serial.println("RTC time is accurate. No update needed.");
    }
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Periodically update RTC from NTP
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 3600000) { // Update setiap 1 jam
      Serial.println("Periodic RTC update from NTP...");
      setRTCFromNTP(true);
      lastUpdate = millis();
    }
  }

  if (rtc.begin()) {
    DateTime now = rtc.now();

    Serial.print("Current Date: ");
    Serial.print(now.year());
    Serial.print("-");
    Serial.print(now.month());
    Serial.print("-");
    Serial.print(now.day());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());

    Jam = now.hour();
    Men = now.minute();
  } else {
    Serial.println("RTC not detected. Using NTP time...");
    timeClient.update();
    Jam = timeClient.getHours();
    Men = timeClient.getMinutes();
  }
  
  char isi[6];
  sprintf(isi, "%02d:%02d", Jam, Men);
  Disp.setFont(Font);
  Disp.drawText(0, 0, isi); 
  Disp.loop();
}
