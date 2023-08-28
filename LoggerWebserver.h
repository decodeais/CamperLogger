#ifndef LoggerWebserver_h
#define LoggerWebserver_h

#include "LoggerMisc.h"
#include <WebServer.h>
#include "defs.h"
#include "LoggerOTA.h"
#include "LoggerWiFi.h"
#include "LoggerSPIFSS.h"

#include "LoggerMisc.h"
void WebServerInit();
void handle_wificonfig();
void handle_savewificonfig();
void handle_root();
void handle_mppt();
void handle_sensors();
void handle_cfg();
void handle_cfgSpecial();
void handle_savecfg();
void handle_json();
void handle_notfound();
void handle_savecfgSpecial();
void handle_measure();




#endif /*Wecbserver_h*/