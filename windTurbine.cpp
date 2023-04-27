
#include <Arduino.h>
#include "defs.h"
#include "LoggerMisc.h"
#include <driver/adc.h>
#include <windTurbine.h>
#include <esp32-hal-adc.h>
#include <remotedebug.h>

#define I_SCALE_5A 2*5.4054054  /// A/V
#define U_SCALE 0.0110294117647*1.082 // 75/6.8*x
#define FILTER 0.3
#define OFFS_FILTER 0.1

//#define ERRORCONV 2

AnaValueStruct AnaValue;

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


void Measure_Analog()
{
static float ActFilter = 1.0;
float I;
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
AnaValue.Iturb = Lowpass((AnaValue.ch_4-AnaValue.ch_5/2.0)/1000*I_SCALE_5A, AnaValue.Iturb, ActFilter) ; 

debugV("Ibatt : %f",AnaValue.Ibatt);
debugV("Iturb : %f",AnaValue.Iturb);



if ( SpecialSettings.TurbineSTOP == true) // calibration when turbine stopped
{
AnaValue.OffsIbatt = Lowpass(AnaValue.Ibatt,AnaValue.OffsIbatt, OFFS_FILTER); //calibration
AnaValue.OffsIturb = Lowpass(AnaValue.Iturb,AnaValue.OffsIturb, OFFS_FILTER);
if (abs(AnaValue.Iturb-AnaValue.OffsIturb) <= 0.01)
{
  calibration = true;
}
}

AnaValue.Ubal = AnaValue.OffsIturb;
//I=(AnaValue.Ibatt - AnaValue.Iturb)/2.0-0.22;I = -(AnaValue.Iturb - AnaValue.OffsIturb);
if ( I < 0.0 ) 
{ 
  I = 0.0; 
}
AnaValue.Pturb = I*AnaValue.Uturb;
AnaValue.Wturb = AnaValue.Pturb*timePassedSince(gnLastUpdate)/1000.0;
gnLastUpdate = millis();
ActFilter = FILTER;
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

