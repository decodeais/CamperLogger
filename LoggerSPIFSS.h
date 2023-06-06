#ifndef LoggerSPIFSS_h
#define LoggerSPIFSS_h

#include <SPIFFS.h>

#include "defs.h"
// Partition scheme: Minimal SPIFFS (1.9MB APP with OTA)/190KB SPIFFS)

#include "esp_log.h"
#include <ctype.h>


#include "LoggerMisc.h"
#include <Update.h>
#include "defs.h"

#include <base64.h>


String SaveSettings(void);
String FileError(int line, const char * fname);
void fileSystemCheck();
void ResetFactory(void);
void ResetCounter(void);
//String SaveSettings(void);

String readFile(String filename);
String LoadSettings();
String SaveToFile(char* fname, int index, byte* memAddress, int datasize);
String LoadFromFile(char* fname, int index, byte* memAddress, int datasize);
void writeFile(fs::FS &fs, const char * path, String message);
String InitFile(const char* fname, int datasize);

#endif /*LOggerSPIFSS_h*/