#include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug
//twtwqtzwqtweqtwetwqt
#include "defs.h"
#include <exteeprom.h>
#include "RemoteDebug.h" 
#include <arduino.h>
#include <stddef.h>
#include <typeinfo>
#include "eeprom.h"

//#define offsetof(s,m)   (size_t)&(((s *)0)->m)

 extern extEEPROM myEEPROM;
extern RemoteDebug Debug;
extern AnaValueStruct AnaValue;

byte *byteoffset;


eepromStruct testdata;
int asdf;
void test()
{
 byte i2cStat = myEEPROM.begin( myEEPROM.twiClock100kHz);
 if ( i2cStat != 0 ) {
 debugE("I2C Problem");
  }


int offs = offsetof(eepromStruct,IturbCal);
//byteoffset = (byte*)&testdata;
testdata.IturbCal=AnaValue.OffsIturb;
debugV("Write to EEprom IturbCal: %f",testdata.IturbCal);
myEEPROM.write(offsetof(eepromStruct,IturbCal),(byte*) &(testdata.IturbCal), sizeof(testdata.IturbCal));
myEEPROM.read(data(eepromStruct,testdata,IturbCal));
debugV("Read to EEprom IturbCal: %f",testdata.IturbCal);

}


//MAKE_TYPE_INFO( eeprom )

//-(byte*)&eeprom;