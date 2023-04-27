#ifndef defs_h
#define defs_h
#include <Arduino.h>

#define VERSION 2.1
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_DEBUG_MORE 4
#define FILE_SECURITY "/security.dat" // WiFi settings
#define FILE_SETTINGS "/settings.dat" // user settings
#define FILE_SPECIALSETTINGS "/specset.dat" // special settings


// TIME SETTINGS
// DST setting is stored in settings struct and is updated from the server
#define NTP_SERVER "time.euro.apple.com" // "pool.ntp.org"
#define TIME_ZONE 60                     // minutes ahead of GMT during winter.

// Geohash
#define GEOHASH_PRECISION 8

// AP password
#define DEFAULT_PASSWORD "loggerconfig"

#define WIFI_RECONNECT_INTERVAL 300 // seconds


// PIN DEFINITIONS
#define PIN_STATUS_LED 2            // LED on ESP32 dev board
#define PIN_EXT_LED 13              // LED on PCB

#define TURBINE_STOP 12              // Output to stopthe turbine
#define CONVERTER_ON 14              // Out put to switch the converter on

#define ERRORCONV 27                // input to read vonverter error output


#define GPS_PIN 27                  // serial
#define VE_DIRECT_PIN_2 16          //  input (RS-232 TTL)
#define ONEWIRE_PIN 2               // one wire input (temperature sensors)

#define I2C_SDA 21                  // I2C data Pin
#define I2C_SCL 22                  // I2C clock Pin

#define DEVICE_MPPT 2 // MPPT output has only one block
#define CONFIG_FILE_VERSION 5



typedef struct 
{
  char WifiSSID[32];
  char WifiKey[64];
  char WifiSSID2[32];
  char WifiKey2[64];
  char WifiAPKey[64];
  char Password[26];
  // its safe to extend this struct, up to 4096 bytes, default values in config are 0
}SecurityStruct;

typedef struct   {
  uint8_t Second;
  uint8_t Minute;
  uint8_t Hour;
  uint8_t Wday;   // day of week, sunday is day 1
  uint8_t Day;
  uint8_t Month;
  uint8_t Year;   // offset from 1970;
}timeStruct;



typedef struct {
  int config_file_version;
  byte DST;
  bool upload_get;
  char upload_get_host[64];
  bool upload_get_ssl;
  int upload_get_port;
  bool upload_influx;
  int influx_port;
  bool influx_ssl;
  // changes for InfluxDB 2.x
  char influx_host[96];
  char influx_bucket[20];
  char influx_org[20];
  char influx_token[100];
  char influx_user[16];
  //
  bool influx_write_temp;
  bool influx_write_geohash;
  bool influx_write_coords;
  bool influx_write_speed_heading;
  int gps_upload_interval;
  int readings_upload_interval;
  bool influx_write_mppt;
  bool influx_write_gps;
}SettingsStruct;

typedef struct 
{
  // Temperature sensors
  float temp[10];         // max 10 temperature sensors (deg C)
                          // MPPT vars
  float MPPT_ytot;        // MPPT yield total (kWh)    H19
  float MPPT_yday;        // MPPT yield today (kWh)    H20
  int MPPT_Pmax;          // MPPT max power today (W)  H21
  int MPPT_err;           // MPPT error number         ERR
  int MPPT_state;         // MPPT state                CS
  float MPPT_Vbatt;       // MPPT output voltage (V)   V
  float MPPT_Ibatt;       // MPPT output current (A)   I
  float MPPT_Vpv;         // MPPT input voltage (V)    VPV
  int MPPT_Ppv;           // MPPT input power (W)      PPV
  bool MPPT_ok;           // MPPT checksum on last block OK
  String MPPT_PID;        // MPPT Product ID
  String MPPT_serial;     // MPPT serial number
  bool MPPT_load_on;      // MPPT load output status
  float MPPT_Iload;       // MPPT load current
  bool MPPT_has_load = 0; // MPPT has load output
  // GPS readings
  String GPS_fix;           // GPS status (active/timeout/void)
  String GPS_date;          // GPS date DDMMYY
  String GPS_time;          // GPS time HHMMSSCC (in UTC!)
  String GPS_lat;           // GPS latitude (0...90 N or S)
  String GPS_lat_abs;       // GPS latitude (-90...90)
  String GPS_lon;           // GPS longitude (0...180 E or W)
  String GPS_lon_abs;       // GPS longitude (-180...180)
  String GPS_speed;         // GPS speed in km/h
  String GPS_alt;           // GPS altitude in m
  String GPS_sat;           // GPS amount of satelites
  String GPS_dop;           // GPS dop/hdop
  String GPS_heading;       // GPS heading (in deg, 0 if not moving)
  String GPS_geohash;       // Geohash
  String GPS_GPRMCsentence; // komplete sentences from GPS
  String GPS_GPVTGsentence; // komplete sentences from GPS
  String GPS_GPGGAsentence; // komplete sentences from GPS

} readingsStruct;

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
float Pturb;
float Wturb;
float OffsIbatt;
float OffsIturb;
}AnaValueStruct;

typedef struct {
bool TurbineSTOP=true;
bool ConverterON=false;
bool ConverterAuto;
float U_ConverterON;
float U_ConverterOFF;
bool TurbineAuto;
float U_TurbineSTOP;
float U_TurbineRUN;
} SpecialSettingsStruct;

#endif /*defs_h*/