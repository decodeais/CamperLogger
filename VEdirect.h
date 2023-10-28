#ifndef VEdirect_h
#define VEdirect_h

#include <Arduino.h>

#include <Update.h>
#include "defs.h"
#include "LoggerMisc.h"
#include <base64.h>

void readVEdirect(int device);
void readVEdirect_Converter();
void parseMPPT(String line);
void parseConverter(String line);
byte calcChecksum(String input);


#endif