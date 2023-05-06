
#include <Arduino.h>
#include "defs.h"
#include "LoggerMisc.h"
#include <driver/adc.h>
#include <windTurbine.h>
#include <esp32-hal-adc.h>
#include <remotedebug.h>
#include <exteeprom.h>
#include  "eeprom.h"


#define I_SCALE_5A 2*5.4054054  /// A/V
#define U_SCALE 0.0110294117647*1.082 // 75/6.8*x
#define FILTER 0.2
#define OFFS_FILTER 0.1

//#define ERRORCONV 2
 
AnaValueStruct AnaValue;
float TempInt;
float Wtemp;
extern eepromStruct testdata;
extern extEEPROM myEEPROM;
/*Inputs*/
bool ErrorConverter;
bool calibration = false;
SpecialSettingsStruct SpecialSettings;
bool stopTurbine;
bool runConverter;

void Init_Analog()
{
  // Set AD-coverter 4095 Bit resolution
  analogReadResolution(12);

  //ADC_ATTEN_11db gives full-scale voltage 3.9 V
  // voltage/2 resistor = 7.8V 
  analogSetAttenuation(ADC_11db);


  analogSetClockDiv(2);                 // Set the divider for the ADC clock, default is 1, range is 1 - 255
  pinMode(TURBINE_STOP, OUTPUT);
  pinMode(CONVERTER_ON, OUTPUT);
  pinMode(ERRORCONV, PULLUP);
}

float Lowpass(float NewValue, float OldValue,float FilterKoeff)
{
  return OldValue*(1.0-FilterKoeff)+ NewValue*FilterKoeff;
}

void ResCount()
{
  testdata.Energie = 0;
myEEPROM.write(offsetof(eepromStruct,Energie),(byte*) &(testdata.Energie), sizeof(testdata.Energie));
myEEPROM.read(data(eepromStruct,testdata,Energie));
 AnaValue.Wturb=	testdata.Energie;

}

void Measure_Analog()
{
static float ActFilter = 1.0;
float I;
float timePassed;
 static long int gnLastUpdate = millis();
    analogReadResolution(12);
    AnaValue.ch_0 = analogReadMilliVolts(36);
    AnaValue.ch_3 = analogReadMilliVolts(39);
    AnaValue.ch_6 = analogReadMilliVolts(34);
    AnaValue.ch_7 = analogReadMilliVolts(35);
    AnaValue.ch_4 = analogReadMilliVolts(32);
    AnaValue.ch_5 = analogReadMilliVolts(33);
/*    
AnaValue.Uvcc = AnaValue.ch_5*0.002;
AnaValue.Ubatt = AnaValue.ch_3*U_SCALE;
AnaValue.Ubal = AnaValue.ch_0*U_SCALE;
AnaValue.Uturb = AnaValue.ch_6*U_SCALE;
*/
extern RemoteDebug Debug;
AnaValue.Uvcc = Lowpass(AnaValue.ch_5*0.002, AnaValue.Uvcc, ActFilter);
AnaValue.Ubatt = Lowpass(AnaValue.ch_3*U_SCALE,AnaValue.Ubatt, ActFilter);
AnaValue.Ubal = Lowpass(AnaValue.ch_0*U_SCALE,AnaValue.Ubal, ActFilter);
AnaValue.Uturb = Lowpass(AnaValue.ch_6*U_SCALE,AnaValue.Uturb, ActFilter);

debugV("Uvcc : %f",AnaValue.Uvcc);
debugV("Ubatt : %f",AnaValue.Ubatt);
debugV("Ubal : %f",AnaValue.Ubal);
debugV("Uturb : %f",AnaValue.Uturb);

/*
AnaValue.Ibatt = (AnaValue.ch_7*2.0/1000.0-AnaValue.Uvcc/2.0)*I_SCALE_5A; 
AnaValue.Iturb = (AnaValue.ch_4*2.0/1000.0-AnaValue.Uvcc/2.0)*I_SCALE_5A;
*/
AnaValue.Ibatt = Lowpass((AnaValue.ch_7-AnaValue.ch_5/2.0)/1000*I_SCALE_5A, AnaValue.Ibatt, ActFilter) ; 






if ( SpecialSettings.TurbineSTOP == true) // calibration when turbine stopped
{
AnaValue.Iturb = Lowpass((AnaValue.ch_4-AnaValue.ch_5/2.0)/1000*I_SCALE_5A, AnaValue.Iturb, ActFilter/10) ; 
//AnaValue.OffsIbatt = Lowpass(AnaValue.Ibatt,AnaValue.OffsIbatt, OFFS_FILTER); //calibration
AnaValue.OffsIturb = Lowpass(AnaValue.Iturb,AnaValue.OffsIturb, OFFS_FILTER);
if (abs(AnaValue.Iturb-AnaValue.OffsIturb) <= 0.005)
{
  calibration = true;
}
}
else
{
  AnaValue.Iturb = Lowpass((AnaValue.ch_4-AnaValue.ch_5/2.0)/1000*I_SCALE_5A, AnaValue.Iturb, ActFilter) ; 

}
debugV("Ibatt : %f",AnaValue.Ibatt);
debugV("Iturb : %f",AnaValue.Iturb);
AnaValue.Ubal = AnaValue.OffsIturb;
//I=(AnaValue.Ibatt - AnaValue.Iturb)/2.0-0.22;
debugV("Iturb : %f",AnaValue.Iturb);
debugV("OffsIturb : %f",AnaValue.OffsIturb);
I = -(AnaValue.Iturb - AnaValue.OffsIturb);
if ( I < 0.0 ) 
{ 
  I = 0.0; 
}
AnaValue.Pturb = I*AnaValue.Uturb;
debugV("I : %f",I);
debugV("Uturb : %f",AnaValue.Uturb);
timePassed = timePassedSince(gnLastUpdate);
debugV("Pturb : %f",AnaValue.Pturb);
Wtemp += AnaValue.Pturb*timePassed/1000.0;
debugV("TimePassed : %f",timePassed);
debugV("Wtemp : %f",Wtemp);
//AnaValue.Wturb

if  (Wtemp > 1000.0)
{
  Wtemp = modf (Wtemp , &TempInt); // fractpart = modf (param , &intpart);
  debugV("Wtemp : %f",Wtemp);                                                                                                                      
//AnaValue.Wturb += TempInt;
testdata.Energie += TempInt;
myEEPROM.write(offsetof(eepromStruct,Energie),(byte*) &(testdata.Energie), sizeof(testdata.Energie));
}           
AnaValue.Wturb=(testdata.Energie+Wtemp)/3600;
debugV("Wturb : %f",AnaValue.Wturb);
//testdata.Energie = AnaValue.Wturb;



gnLastUpdate = millis();
ActFilter = FILTER;  // only ther first cycle is without Lowpass
}


void read_inputs()
{
ErrorConverter = digitalRead(ERRORCONV) ;
}

void control()
{
  if (calibration)   // run turbine only if current is calibrated
  {
    if (SpecialSettings.TurbineAuto )
    {
      if (AnaValue.Uturb < SpecialSettings.U_TurbineRUN )
      {
      SpecialSettings.TurbineSTOP = false;
      }
      if (AnaValue.Uturb > SpecialSettings.U_TurbineSTOP )
      {
      SpecialSettings.TurbineSTOP = true;
      calibration = false; // turbine stop is chance for recalibration
      }
    }
  }
  else
  {
    SpecialSettings.TurbineSTOP = true;
  }
  if (SpecialSettings.ConverterAuto )
  {
    if (SpecialSettings.U_ConverterON < AnaValue.Ubatt)
    {
    SpecialSettings.ConverterON = true;
    }
    if (SpecialSettings.U_ConverterOFF > AnaValue.Ubatt)
    {
    SpecialSettings.ConverterON = false;
    }
  }

}


void writeOutputs()
{
  digitalWrite(TURBINE_STOP,SpecialSettings.TurbineSTOP);
  digitalWrite(CONVERTER_ON,SpecialSettings.ConverterON);
}

