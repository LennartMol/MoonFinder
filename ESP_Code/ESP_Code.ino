#include <WiFi.h>
#include "time.h"
#include "sntp.h"

// ESP32 GND - GND Arduino
// ESP32 TX0 (tussen RX0 en D22) - 10 Arduino (of zelf ff kiezen en aanpassen in code van Arduino)

const char* ssid       = "Ziggo0581713";
const char* password   = "EdV6mbpudz3r";

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

void sendTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  }
  char buffer[25]; // Allocate a buffer to store the formatted string
  strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &timeinfo);
  String dateTime = String(buffer);
  Serial.println(dateTime);
}

void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  sendTime();
}

void setup()
{
  Serial.begin(115200);
  sntp_set_time_sync_notification_cb( timeavailable );
  sntp_servermode_dhcp(1);   
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");

}

void loop()
{
  delay(3000);
  sendTime(); 
}
