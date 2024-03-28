#include "LoggerMisc.h"
#include "VEdirect.h"
//#include "Sensors.h"
#include "LoggerMisc.h"

extern bool pause_background_tasks;
extern bool background_tasks_paused;
extern bool firstbgrun;
extern bool read_ve_direct_mppt;
extern bool read_ve_direct_converter;

extern HardwareSerial Serial_VE_MPPT;
extern HardwareSerial Serial_VE_Converter;
extern String inventory;
extern bool GPS_present;
extern int nr_of_temp_sensors;
extern bool inventory_complete;
extern readingsStruct readings;



void runBackgroundTasks() {
  // These tasks will run in the background.

  // pause background tasks during data upload.
  int paused_seconds = 0;
  while (pause_background_tasks) {
    if (paused_seconds > 60) {
      addLog(LOG_LEVEL_ERROR, "BACKGR: Background tasks paused for more than 60 seconds. Rebooting");
      ESP.restart();
    }
    background_tasks_paused = 1;
    addLog(LOG_LEVEL_INFO, "BCKGR: Background tasks paused");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    paused_seconds++;
  }
  //  while(WiFi.status() != WL_CONNECTED) {
  //    addLog(LOG_LEVEL_INFO, "BCKGR: Not running background tasks, WiFi not connected.");
  //    vTaskDelay(5000 / portTICK_PERIOD_MS);
  //  }
  if (background_tasks_paused == 1) {
    addLog(LOG_LEVEL_INFO, "BCKGR: Resuming background tasks");
    background_tasks_paused = 0;
  }

  // In case something goes wrong, this buys us time to download a new image.
  if (firstbgrun) {
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    addLog(LOG_LEVEL_INFO, "BCKGR: Delaying first background run");
    vTaskDelay(20000 / portTICK_PERIOD_MS);
    firstbgrun = 0;
  }
  
 // Read Victron MPPT charge controller
  if (read_ve_direct_mppt) {
    Serial_VE_MPPT.begin(19200, SERIAL_8N1, VE_DIRECT_PIN_2, /*jps(int8_t)*/1, true);
   
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
    readVEdirect(DEVICE_MPPT);

    vTaskDelay(10 / portTICK_PERIOD_MS);
    Serial_VE_MPPT.end();
  }

  // Read Victron MPPT charge controller
  if (read_ve_direct_converter) {
    Serial_VE_Converter.begin(19200, SERIAL_8N1, VE_DIRECT_PIN_1, /*jps(int8_t)*/1, true);
   
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
    readVEdirect_Converter();

    vTaskDelay(10 / portTICK_PERIOD_MS);
    Serial_VE_Converter.end();
  }
  
  vTaskDelay(10 / portTICK_PERIOD_MS);
  //readGPS();

  vTaskDelay(10 / portTICK_PERIOD_MS);
  //readTemperatureSensors();

  // Now that we have read all devices, build inventory list
  inventory = "";
/*
  if (GPS_present) {
    inventory += "GPS module detected";
    inventory += "\n\n";
  }
*/
/*
  nr_of_temp_sensors = 0;
  for (int i = 0; i < 10; i++) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
    if (readings.temp[i] != -127) {
      nr_of_temp_sensors++;
    }
  }
  if (nr_of_temp_sensors > 0) {
    inventory += String(nr_of_temp_sensors) + " 1Wrire temperature sensor(s)\n";
  }
  if (inventory.length() == 0) {
    inventory = "No devices detected";
  }
  inventory_complete = 1;
  */
}
