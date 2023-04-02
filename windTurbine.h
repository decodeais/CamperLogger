#ifndef windTurbine_h
#define winTurbine_h



typedef struct  {
  float ch_0;
  float ch_3;
  float ch_6;
  float ch_7;
  float ch_4;
  float ch_5;
  float ch_8;
  float ch_9;
  

float Ubatt;
float Ubal;
float Uturb;
float Ibatt;
float Iturb;
float Uvcc;
}AnaValueStruct;

typedef struct {
bool TurbineSTOP;
bool ConverterON;
bool ConverterAuto;
float U_ConverterON;
float U_ConverterOFF;
bool TurbineAuto;
float U_TurbineSTOP;
float U_TurbineRUN;
} SpecialSettingsStruct;


void Measure_Analog();

void Init_Analog();

#endif /* winTurbine*/