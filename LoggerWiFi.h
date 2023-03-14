
#ifndef LoggerWiFi_h
#define LoggerWiFi_h


#include "defs.h"

#include <WiFi.h>
#include <WiFiUdp.h>

#include "LoggerMisc.h"



String WifiGetAPssid();
void WifiAPconfig();
void WifiAPMode(boolean);
bool WifiIsAP();
boolean WifiConnect(byte);
boolean WifiConnectSSID(char, char, byte);
int getWiFiStrength(int);
void updateAPstatus();

#endif /*LoggerWiFi*/