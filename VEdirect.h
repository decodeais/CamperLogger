#ifndef VEdirect_h
#define VEdirect_h

#include <Arduino.h>

#include <Update.h>
#include "defs.h"
#include "LoggerMisc.h"
#include <base64.h>

void readVEdirect(int device);
void parseMPPT(String line);
byte calcChecksum(String input);


#endif