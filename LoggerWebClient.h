#ifndef LoggerWebClient_h
#define LoggerWebClient_h


#include "defs.h"
#include <base64.h>
#include "LoggerMisc.h"
#include <rom/rtc.h>
#include "LoggerOTA.h"
#include <HTTPClient.h>
#include "LoggerWebClient.h"
#include "LoggerSPIFSS.h"
//#include "LoggerSPIFSS"



void reportResetReason();
void callHome();
String getVarFromString(String var, String cfgData);
String urlOpen(String path, String query);
String httpGet(String path, String query, int port = 80) ;
String httpsGet(String path, String query, int port = 443);
void uploadFile(String content, String type);
void sendDataToLogServer();




#endif /*LoggerwebClient_h*/