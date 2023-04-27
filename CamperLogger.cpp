/*
    1-WIRE            GPIO 26
    GPS               GPIO 27
*/

// Partition scheme: Minimal SPIFFS (1.9MB APP with OTA)/190KB SPIFFS)
#include "GPS.h"
#include "defs.h"

#include "VEdirect.h"
#include "BackgroundTasks.h"
#include "LoggerMisc.h"
#include "LoggerWiFi.h"
#include "LoggerWebserver.h"
#include "LoggerWebClient.h"
#include "DataUpload.h"
#include "LoggerSPIFSS.h"


#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <base64.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
//influxDB2
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug

#include <windTurbine.h>
extern AnaValueStruct AnaValue;

static float fileversion = 2.1;
static String verstr = "Version 2.1";  //Make sure we can grep version from binary image

// Changing this number may reset all settings to default!
#define CONFIG_FILE_VERSION 5
RemoteDebug Debug;
String temp;
//*************************************************************************
// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient influxclient;
// Declare Data point
Point sensor("MPPT");
Point Mturb("TURBINE");
//*********************************************************************jps

SettingsStruct Settings;

String getFileChecksum(String);

byte logLevel = 4;  // not a #define, logLevel can be overridden from the server



TaskHandle_t BackgroundTask;

SecurityStruct  SecuritySettings;

readingsStruct readings;

// DATA COLLECTION VARIABLES

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
unsigned long timerConnectionCheck = millis() + 60000L;
unsigned long nextWifiRetry = millis() + WIFI_RECONNECT_INTERVAL * 1000;

// prototypes with default ports for http and https
//String httpsGet(String path, String query, int port = 443);
//String httpGet(String path, String query, int port = 80);

//OneWire oneWire(ONEWIRE_PIN);

WebServer Webserver(80);

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


void test_I2C() {
  byte error, address;
  int nDevices;
  //Serial.println("I2C Scanning...");
  debugV("I2C Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      debugV("I2C device found at address 0x%x",HEX);
      nDevices++;
    }
    else if (error==4) {
      debugE("Unknow error at address 0x%x",HEX);
    }    
  }
  if (nDevices == 0) {
    debugE("No I2C devices found\n");
  }
  else {
    debugV("done\n");
  }
  delay(5000);          
}


void setup() {




  Wire.begin(I2C_SDA, I2C_SCL,10000);
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_EXT_LED, OUTPUT);
  pinMode(GPS_PIN, INPUT);
  pinMode(VE_DIRECT_PIN_2, INPUT);
  pinMode(21, INPUT); 
   pinMode(22, INPUT);
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

Init_Analog();


  Serial.begin(115200);
  SerialGPS.begin(9600, SERIAL_7E1, GPS_PIN, -1, false);

 // addLog(LOG_LEVEL_INFO, "CORE : Version " + String(fileversion, 3) + " starting");

 // addLog(LOG_LEVEL_DEBUG, "CORE : Size of stettins struct: " + String(sizeof(SettingsStruct)));
 //debugV("CORE : Size of settings struct: %d", sizeof(SettingsStruct));
  for (byte x = 0; x < 16; x++)
    ledChannelPin[x] = -1;

  // get chip MAC
  esp_efuse_mac_get_default(chipid);
  sprintf(chipMAC, "%02x%02x%02x%02x%02x%02x", chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);

  fileSystemCheck();

  LoadSettings();

  //addLog(LOG_LEVEL_INFO, "CORE : DST setting: " + String(Settings.DST));

  WifiAPconfig();
  WifiConnect(3);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WIFI : WiFi connected, disabling AP in 10 seconds");
    timerAPoff = millis() + 10000L;
  }
  delay(100);
  WebServerInit();

  
 /* http://remotedebugapp.pluggable.biz*/
  MDNS.addService("telnet", "tcp", 23); // Telnet server of RemoteDebug, register as telnet
  Debug.begin("HOST_NAME"); // Initialize the WiFi server
  Debug.setResetCmdEnabled(true); // Enable the reset command

	Debug.showProfiler(true); // Profiler (Good to measure times, to optimize codes)

	Debug.showColors(true); // Colors
  
debugI("Ready");
  debugI("IP address: %s",WiFi.localIP());

  reportResetReason();  // report to the backend what caused the reset
  callHome();           // get settings and current software version from server
  debugV( "CORE : Setup done. Starting main loop");

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
      debugI("Start updating %s",type);
    })
    .onEnd([]() {
      debugI("End");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      debugI("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
        debugE("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)
        debugE("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)
        debugE("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)
        debugE("Receive Failed");
      else if (error == OTA_END_ERROR)
        debugE("End Failed");
    });
  ArduinoOTA.begin();



Debug.handle();
 test_I2C(); 
  

	

}

void loop() {

Debug.handle();



  void read_inputs();
  // check if AP should be turned on or off.
  updateAPstatus();
  // process incoming requests
  Webserver.handleClient();

  // Upload non-GPS readings
  if (Settings.influx_write_mppt) {
    if (timerLog != 0 && timeOutReached(timerLog) && WiFi.status() == WL_CONNECTED) {
      influxclient.setConnectionParams(Settings.influx_host, Settings.influx_org, Settings.influx_bucket, Settings.influx_token, InfluxDbCloud2CACert);
      sensor.clearFields();
      Mturb.clearFields();
      timerLog = millis() + Settings.readings_upload_interval * 1000L;
      pause_background_tasks = 1;
      while (!background_tasks_paused) {
        delay(100);
      }

      debugI("CORE : Uploading readings");
      digitalWrite(PIN_EXT_LED, HIGH);
      //callHome();
      //uploadGetData();
            Serial.print("Reading ADC ");
            Measure_Analog();
            control();
      uploadInfluxReadings();
      Serial.print("Writing MPPT: ");
      Serial.println(sensor.toLineProtocol());
      // Write point
      if (!influxclient.writePoint(sensor)) {
        Serial.print("InfluxDB MPPT write failed: ");
        Serial.println(influxclient.getLastErrorMessage());
      }
      Serial.print("Writing Mturb: ");
      Serial.println(Mturb.toLineProtocol());
      if (!influxclient.writePoint(Mturb)) {
        Serial.print("InfluxDB Mturb write failed: ");
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
      debugI("CORE : Uploading GPS data");
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
    Serial.println("WIFI : Not connected, trying to connect");
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


if (timerConnectionCheck != 0 && timeOutReached(timerConnectionCheck) && WiFi.status() == WL_CONNECTED) {
      timerConnectionCheck = millis() + 60000L;
  if (influxclient.validateConnection()) {
    temp = "Connected to InfluxDB:"+influxclient.getServerUrl();
    
    Debug.println(temp);
    } 
    else {
    debugE("InfluxDB connection failed: ");
    debugE("agaagga  %.96s",Settings.influx_host);
    debugE("Organisation : %.20s",Settings.influx_org);
    debugE("Bucket : %.20s",Settings.influx_bucket);
    debugE("Token : %.100s",Settings.influx_token);
    temp ="Error : "+influxclient.getLastErrorMessage();
    Debug.println(temp);
Debug.println("Error : "+influxclient.getLastErrorMessage());

    /*Serial.printf("error : %s",influxclient.getLastErrorMessage());
    Serial.printf("host : %s",Settings.influx_host);
    Serial.println(Settings.influx_org);
    Serial.println(Settings.influx_bucket);
    Serial.println(Settings.influx_token);*/
  }  
  writeOutputs();
  


}
}