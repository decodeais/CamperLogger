#ifndef Misc_h
#define Misc_h

#include <FS.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <rom/rtc.h>
#include "esp_log.h"
#include <ctype.h>
#include <WiFi.h>
#include <WebServer.h>
// for OTA from IDE
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//#include <TinyGPSPlus.h>



#include <Arduino.h>
#include <ctype.h>
#include "defs.h"
#include <WiFiClientSecure.h>




/********************************************************************************************\
  Time stuff
  \*********************************************************************************************/
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
#define DAYS_PER_WEEK (7UL)
#define SECS_PER_WEEK (SECS_PER_DAY * DAYS_PER_WEEK)
#define SECS_PER_YEAR (SECS_PER_WEEK * 52UL)
#define SECS_YR_2000  (946684800UL) // the time at the start of y2k
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
#define RTC_BASE 65 // system doc says user area starts at 64, but it does not work (?)

void addLog(byte level, String line);
boolean timeOutReached(unsigned long timer);
long timePassedSince(unsigned long timestamp);
unsigned long now();
void initTime();
void setTime(unsigned long t);
void breakTime(unsigned long timeInput, timeStruct &tm);
int weekday();
String formattedTime();
String formattedDate();
void saveToRTC(byte Par1);
boolean readFromRTC(byte* data);

void statusLED(boolean traffic);

#endif /*Misc_h*/