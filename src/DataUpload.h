#ifndef DataUpload_h
#define Dataupload_h

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "defs.h"
#include "LoggerMisc.h"
#include "LoggerWebClient.h"


void uploadInfluxReadings();
void uploadGetData();

#endif /*DataUpload_h*/