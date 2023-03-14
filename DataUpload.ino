
#include "DataUpload.h"





void uploadInfluxReadings();
void uploadGetData();


extern String httpGet(String path, String query, int port);
//extern Point  sensor("MPPT");
extern Point  sensor;
extern SettingsStruct Settings;
extern readingsStruct readings;
extern bool inventory_complete;
extern int nr_of_temp_sensors;
extern bool inventory_requested;
extern bool GPS_present;
extern String inventory;



void uploadInfluxReadings() {
  if (!Settings.upload_influx) {
    return;
  }

  addLog(LOG_LEVEL_INFO, "DATA : Writing data to influxdb");

  // this is where the readings are being written to influxdb.
  // You can add or remove any readings below. For MPPT and BMV readings,
  // check readings.MPPT_ok and readings.BMV_B1_ok respectively to make
  // you are not pushing corrupted data into influxdb. If you are writing
  // readings.BMV_LDD, check readings.BMV_B2_ok (this reading is in the
  // second block of the BMV output).

  if (readings.MPPT_ok && Settings.influx_write_mppt) {
    sensor.addField("Ppv", readings.MPPT_Ppv);
    sensor.addField("Vpv", readings.MPPT_Vpv);
    sensor.addField("Ibat", readings.MPPT_Ibatt);
    sensor.addField("Vbat", readings.MPPT_Vbatt);
    sensor.addField("State", readings.MPPT_state);
    sensor.addField("Error", readings.MPPT_err);
    sensor.addField("Pmax", readings.MPPT_Pmax);
    sensor.addField("Yield_today", readings.MPPT_yday);
    //sensor.addField("Yield_yesterday", readings.MPPT_yyesterd); 
    sensor.addField("Yield_total", readings.MPPT_ytot);
  }
  // load status and load current are not available on all MPPT's, values should
  // only be written to influxdb if they are present.
  if (readings.MPPT_ok && Settings.influx_write_mppt && readings.MPPT_has_load) {
    sensor.addField("I_load", readings.MPPT_Iload);
    sensor.addField("RelaisLoad", readings.MPPT_load_on);
  }


  if (Settings.influx_write_temp) {
    for (int i = 0; i < 10; i++) {
      if (readings.temp[i] != -127) {
        //jps     influx_post("Temp" + String(i), String(readings.temp[i]));
      }
    }
  }
}




void uploadGetData() {
  if (inventory_requested && inventory_complete) {
    addLog(LOG_LEVEL_DEBUG, "DATA : Uploading inventory: " + inventory);
    uploadFile(inventory, "inventory");
    inventory_complete = 0;
    inventory_requested = 0;
  }

  if (!Settings.upload_get) {
    return;
  }

  addLog(LOG_LEVEL_INFO, "DATA : Uploading readings to server");

  String request = "";
  for (int i = 0; i < 10; i++) {
    if (readings.temp[i] != -127) {
      request += "&T" + String(i) + "=" + String(readings.temp[i]);
    }
  }

  if (GPS_present) {
    request += "&GPSdate=" + String(readings.GPS_date);
    request += "&GPStime=" + String(readings.GPS_time);
    request += "&GPSlat=" + String(readings.GPS_lat);
    request += "&GPSlon=" + String(readings.GPS_lon);
    request += "&GPSspeed=" + String(readings.GPS_speed);
    request += "&GPSheading=" + String(readings.GPS_heading);
    request += "&GPSfix=" + String(readings.GPS_fix);
    request += "&GPS_geohash=" + String(readings.GPS_geohash);
  }
  String response;
  if (Settings.upload_get_ssl) {
    
    response = httpGet("/update/", request, Settings.upload_get_port);
  } else {
    response = httpGet("/update/", request, Settings.upload_get_port);
  }

  addLog(LOG_LEVEL_DEBUG, "DATA : Response from server: " + response);
}
