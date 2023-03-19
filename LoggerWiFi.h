
#ifndef LoggerWiFi_h
#define LoggerWiFi_h
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <Arduino.h>
#include "defs.h"
#include "LoggerMisc.h"
//#include <WiFiClientSecure.h>

#include <ctype.h>
String WifiGetAPssid();
void WifiAPconfig();
void WifiAPMode(boolean);
bool WifiIsAP();
boolean WifiConnect(byte);
boolean WifiConnectSSID(char*,char*, byte);
int getWiFiStrength(int);
void updateAPstatus();

#endif /*LoggerWiFi*/