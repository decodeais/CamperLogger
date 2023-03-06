/*
    1-WIRE            GPIO 26
    GPS               GPIO 27
*/

// Partition scheme: Minimal SPIFFS (1.9MB APP with OTA)/190KB SPIFFS)

#include <FS.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <rom/rtc.h>
#include "esp_log.h"
#include <ctype.h>
#include <WiFi.h>
#include <WebServer.h>
// for OTA from IDE
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <base64.h>
#include <TinyGPSPlus.h>

//jps influxDB2
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

static float version = 2.1;
static String verstr = "Version 2.1";  //Make sure we can grep version from binary image

// Changing this number may reset all settings to default!
#define CONFIG_FILE_VERSION 5

typedef struct SettingsStruct {
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
};
//*************************************************************************
// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient influxclient;
// Declare Data point
Point sensor("MPPT");
//*********************************************************************jps

SettingsStruct Settings;

String getFileChecksum(String);


#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_DEBUG_MORE 4

byte logLevel = 4;  // not a #define, logLevel can be overridden from the server

#define FILE_SECURITY "/security.dat"  // WiFi settings
#define FILE_SETTINGS "/settings.dat"  // user settings

// TIME SETTINGS
// DST setting is stored in settings struct and is updated from the server
#define NTP_SERVER "time.euro.apple.com"  // "pool.ntp.org"
#define TIME_ZONE 60                      // minutes ahead of GMT during winter.

// Geohash
#define GEOHASH_PRECISION 8

// AP password
#define DEFAULT_PASSWORD "loggerconfig"

// PIN DEFINITIONS
#define PIN_STATUS_LED 2             // LED on ESP32 dev board
#define PIN_EXT_LED 13               // LED on PCB
#define WIFI_RECONNECT_INTERVAL 300  // seconds
#define GPS_PIN 27                   // serial
#define VE_DIRECT_PIN_2 16           //  input (RS-232 TTL)
#define ONEWIRE_PIN 2                // one wire input (temperature sensors)

#define DEVICE_MPPT 2  // MPPT output has only one block

TaskHandle_t BackgroundTask;

struct SecurityStruct {
  char WifiSSID[32];
  char WifiKey[64];
  char WifiSSID2[32];
  char WifiKey2[64];
  char WifiAPKey[64];
  char Password[26];
  //its safe to extend this struct, up to 4096 bytes, default values in config are 0
} SecuritySettings;

struct readingsStruct {
  // Temperature sensors
  float temp[10];          // max 10 temperature sensors (deg C)
                           // MPPT vars
  float MPPT_ytot;         // MPPT yield total (kWh)    H19
  float MPPT_yday;         // MPPT yield today (kWh)    H20
  int MPPT_Pmax;           // MPPT max power today (W)  H21
  int MPPT_err;            // MPPT error number         ERR
  int MPPT_state;          // MPPT state                CS
  float MPPT_Vbatt;        // MPPT output voltage (V)   V
  float MPPT_Ibatt;        // MPPT output current (A)   I
  float MPPT_Vpv;          // MPPT input voltage (V)    VPV
  int MPPT_Ppv;            // MPPT input power (W)      PPV
  bool MPPT_ok;            // MPPT checksum on last block OK
  String MPPT_PID;         // MPPT Product ID
  String MPPT_serial;      // MPPT serial number
  bool MPPT_load_on;       // MPPT load output status
  float MPPT_Iload;        // MPPT load current
  bool MPPT_has_load = 0;  // MPPT has load output
  // GPS readings
  String GPS_fix;            // GPS status (active/timeout/void)
  String GPS_date;           // GPS date DDMMYY
  String GPS_time;           // GPS time HHMMSSCC (in UTC!)
  String GPS_lat;            // GPS latitude (0...90 N or S)
  String GPS_lat_abs;        // GPS latitude (-90...90)
  String GPS_lon;            // GPS longitude (0...180 E or W)
  String GPS_lon_abs;        // GPS longitude (-180...180)
  String GPS_speed;          // GPS speed in km/h
  String GPS_alt;            // GPS altitude in m
  String GPS_sat;            // GPS amount of satelites
  String GPS_dop;            // GPS dop/hdop
  String GPS_heading;        // GPS heading (in deg, 0 if not moving)
  String GPS_geohash;        // Geohash
  String GPS_GPRMCsentence;  // komplete sentences from GPS
  String GPS_GPVTGsentence;  // komplete sentences from GPS
  String GPS_GPGGAsentence;  // komplete sentences from GPS

} readings;

// DATA COLLECTION VARIABLES
String lastBlockMPPT = "";
String inventory = "";
bool inventory_complete = 0;
int nr_of_temp_sensors = 0;
bool inventory_requested = 0;

// We are only going to upload readings from connected devices.
// These values will be set to 1 after the first valid reading
// from the respective device is received.
bool GPS_present = 0;
bool MPPT_present = 0;


// toggle different data sources. Do we need this?
bool read_ve_direct_mppt = 1;  // read MPPT or skip it?
bool read_temp = 0;            // read temperature sensors or skip it?
bool read_gps = 0;             // read GPS data or skip it?
bool read_water_level = 0;     // read tank level sensor or skip it?

String Fcrc;
uint8_t ledChannelPin[16];
uint8_t chipid[6];
char chipMAC[12];
IPAddress apIP(192, 168, 4, 1);
IPAddress apNM(255, 255, 255, 0);
IPAddress apGW(0, 0, 0, 0);

//unsigned long timerAPoff    = millis() + 10000L;
unsigned long timerAPoff = 0;
unsigned long timerLog = millis() + 60000L;  // first upload after 60 seconds.
unsigned long timerGPS = millis() + 60000L;
unsigned long nextWifiRetry = millis() + WIFI_RECONNECT_INTERVAL * 1000;

// prototypes with default ports for http and https
String httpsGet(String path, String query, int port = 443);
String httpGet(String path, String query, int port = 80);

OneWire oneWire(ONEWIRE_PIN);

WebServer WebServer(80);

// enable or disable OTA
boolean otaEnabled = false;

// Serial ports
// UART 0 is used for the console. Because the ESP32 has 3 UARTs and we need 3,
// HardwareSerial(2) is switched to the according pin if we want to read BMV or MPPT.
HardwareSerial SerialGPS(1);  // GPS input (NMEA)
HardwareSerial SerialVE(2);   // VE direct connections

//void onRmcUpdate(nmea::RmcData const);
//void onGgaUpdate(nmea::GgaData const);
TinyGPSPlus gpsParser;

/**************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************/

//ArduinoNmeaParser parser(onRmcUpdate, onGgaUpdate);

void backgroundTasks(void* parameter) {
  for (;;) {
    runBackgroundTasks();
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

bool pause_background_tasks = 0;
bool background_tasks_paused = 0;

bool firstbgrun = 1;

void setup() {
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_EXT_LED, OUTPUT);
  pinMode(GPS_PIN, INPUT);
  pinMode(VE_DIRECT_PIN_2, INPUT);
  pinMode(25, INPUT);  // GPIO25 is parallel wired to GPIO39 because we can not use ADC2!

  digitalWrite(PIN_EXT_LED, HIGH);
  delay(500);
  digitalWrite(PIN_EXT_LED, LOW);

  xTaskCreatePinnedToCore(
    backgroundTasks,  // Task function
    "Background",     // Name
    4000,             // Stack size
    NULL,             // Parameter
    1,                // priority
    &BackgroundTask,  // Task handle
    0);               // core

  Serial.begin(115200);
  SerialGPS.begin(9600, SERIAL_7E1, GPS_PIN, -1, false);

  addLog(LOG_LEVEL_INFO, "CORE : Version " + String(version, 3) + " starting");
  addLog(LOG_LEVEL_DEBUG, "CORE : Size of stettins struct: " + String(sizeof(struct SettingsStruct)));
  for (byte x = 0; x < 16; x++)
    ledChannelPin[x] = -1;

  // get chip MAC
  esp_efuse_mac_get_default(chipid);
  sprintf(chipMAC, "%02x%02x%02x%02x%02x%02x", chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);

  fileSystemCheck();

  LoadSettings();

  addLog(LOG_LEVEL_INFO, "CORE : DST setting: " + String(Settings.DST));

  WifiAPconfig();
  WifiConnect(3);
  if (WiFi.status() == WL_CONNECTED) {
    addLog(LOG_LEVEL_DEBUG, "WIFI : WiFi connected, disabling AP in 10 seconds");
    timerAPoff = millis() + 10000L;
  }
  delay(100);
  WebServerInit();


  reportResetReason();  // report to the backend what caused the reset
  callHome();           // get settings and current software version from server
  addLog(LOG_LEVEL_INFO, "CORE : Setup done. Starting main loop");

  //jps***OTA for Arduino IDE *****************************************************
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
        Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)
        Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)
        Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)
        Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)
        Serial.println("End Failed");
    });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // check if AP should be turned on or off.
  updateAPstatus();
  // process incoming requests
  WebServer.handleClient();

  // Upload non-GPS readings
  if (Settings.influx_write_mppt) {
    if (timerLog != 0 && timeOutReached(timerLog) && WiFi.status() == WL_CONNECTED) {
      influxclient.setConnectionParams(Settings.influx_host, Settings.influx_org, Settings.influx_bucket, Settings.influx_token, InfluxDbCloud2CACert);
      sensor.clearFields();
      timerLog = millis() + Settings.readings_upload_interval * 1000L;
      pause_background_tasks = 1;
      while (!background_tasks_paused) {
        delay(100);
      }

      addLog(LOG_LEVEL_INFO, "CORE : Uploading readings");
      digitalWrite(PIN_EXT_LED, HIGH);
      //callHome();
      //uploadGetData();
      uploadInfluxReadings();
      Serial.print("Writing MPPT: ");
      Serial.println(sensor.toLineProtocol());
      // Write point
      if (!influxclient.writePoint(sensor)) {
        Serial.print("InfluxDB MPPT write failed: ");
        Serial.println(influxclient.getLastErrorMessage());
      }
      digitalWrite(PIN_EXT_LED, LOW);
      pause_background_tasks = 0;
    }
  }
  // Upload GPS data
  if (Settings.influx_write_gps) {
    if (timerGPS != 0 && timeOutReached(timerGPS) && WiFi.status() == WL_CONNECTED) {
      timerGPS = millis() + Settings.gps_upload_interval * 100L;
      pause_background_tasks = 1;
      while (!background_tasks_paused) {
        delay(100);
      }
      addLog(LOG_LEVEL_INFO, "CORE : Uploading GPS data");
      digitalWrite(PIN_EXT_LED, HIGH);
      digitalWrite(PIN_EXT_LED, LOW);
      if (GPS_present) {
        // uploadInfluxGPS();
        sendDataToLogServer();
      }
      Serial.println(sensor.toLineProtocol());
      // Write point
      if (!influxclient.writePoint(sensor)) {
        Serial.print("InfluxDB GPS write failed: ");
        Serial.println(influxclient.getLastErrorMessage());
      }
      pause_background_tasks = 0;
    }
  }
  // retry WiFi connection
  if (WiFi.status() != WL_CONNECTED && timeOutReached(nextWifiRetry)) {
    addLog(LOG_LEVEL_INFO, "WIFI : Not connected, trying to connect");
    WifiConnect(3);
    nextWifiRetry = millis() + WIFI_RECONNECT_INTERVAL * 1000;
  }
  delay(100);
  ArduinoOTA.handle();
  //****************************************************************************
  //  setup InfluxDB Energy
  // Check server connection

  //  influxclient.setConnectionParams(Settings.influx_host, Settings.influx_org, Settings.influx_bucket, Settings.influx_token, InfluxDbCloud2CACert);
  //influxclient.setConnectionParams(INFLUXDB_URL,INFLUXDB_ORG, INFLUXDB_BUCKET,INFLUXDB_TOKEN, InfluxDbCloud2CACert);



  if (influxclient.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(influxclient.getServerUrl());

  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(influxclient.getLastErrorMessage());
    Serial.println(Settings.influx_host);
    Serial.println(Settings.influx_org);
    Serial.println(Settings.influx_bucket);
    Serial.println(Settings.influx_token);
  }
}
