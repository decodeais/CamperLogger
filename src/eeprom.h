#ifndef EEPROM_h
#define EEPROM_h


#define data(t,s,m)  offsetof(t,m),(byte*) &(s.m),sizeof(s.m)

typedef struct {
    float  IturbCal;
    unsigned long Energie;
} eepromStruct;

void test();

#endif //eeprom_h