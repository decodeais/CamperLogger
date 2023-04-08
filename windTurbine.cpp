
#include <Arduino.h>
#include "defs.h"
#include <driver/adc.h>
#include <windTurbine.h>
#include <esp32-hal-adc.h>


#define I_SCALE_5A 0.54054054  /// 1/0.185
#define U_SCALE 0.0110294117647 // 75/6.8

//#define TURBINE_STOP 2
//#define CONVERER_ON 2

//#define ERRORCONV 2

AnaValueStruct AnaValue;


/*Inputs*/
bool ErrorConverter;




SpecialSettingsStruct SpecialSettings;
bool stopTurbine;
bool runConverter;

void Init_Analog()
{
 
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

  analogSetClockDiv(2);                 // Set the divider for the ADC clock, default is 1, range is 1 - 255


    pinMode(TURBINE_STOP, OUTPUT);
    pinMode(CONVERTER_ON, OUTPUT);
    pinMode(ERRORCONV, PULLUP);


}





void Measure_Analog()
{
 
    analogReadResolution(12);
    AnaValue.ch_0 = analogReadMilliVolts(36);
    AnaValue.ch_3 = analogReadMilliVolts(39);
    AnaValue.ch_6 = analogReadMilliVolts(34);
    AnaValue.ch_7 = analogReadMilliVolts(35);
    AnaValue.ch_4 = analogReadMilliVolts(32);
    AnaValue.ch_5 = analogReadMilliVolts(33);
    
AnaValue.Uvcc = AnaValue.ch_5*0.002;
AnaValue.Ubatt = AnaValue.ch_0*U_SCALE;
AnaValue.Ubal = AnaValue.ch_3*U_SCALE;
AnaValue.Uturb = AnaValue.ch_6*U_SCALE;



AnaValue.Ibatt = (AnaValue.ch_7*2/1000-AnaValue.Uvcc/2)*I_SCALE_5A; 
AnaValue.Iturb = (AnaValue.ch_4*2/1000-AnaValue.Uvcc/2)*I_SCALE_5A; 
}


void read_inputs()
{
ErrorConverter = digitalRead(ERRORCONV) ;
}

void control()
{
  if (SpecialSettings.TurbineAuto = true)
  {
    if (SpecialSettings.U_TurbineRUN < AnaValue.Uturb)
    {
     SpecialSettings.TurbineSTOP = false;
    }
    if (SpecialSettings.U_TurbineSTOP > AnaValue.Uturb)
    {
     SpecialSettings.TurbineSTOP = true;
    }
  }
  if (SpecialSettings.ConverterAuto = true)
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

