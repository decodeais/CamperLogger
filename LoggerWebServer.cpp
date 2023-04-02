#include "LoggerWebserver.h"
#include "windTurbine.h"

extern WebServer Webserver;
extern SecurityStruct SecuritySettings;
extern SettingsStruct Settings;
extern bool MPPT_present;
extern String lastBlockMPPT;
extern SettingsStruct settings;
extern readingsStruct readings;
extern unsigned long timerAPoff;

extern AnaValueStruct AnaValue;
extern SpecialSettingsStruct SpecialSettings;

String html_head = 
"<html><head><title>Camper Control</title><style>body {font-family: verdana;background-color: white; color: black;font-size: 40px;}td,th{font-size: 40px;text-align:left;}input[type=checkbox]{padding:2px;transform:scale(3);}input[type=password],select,input[type=submit],input[type=text],input[type=number]{-webkit-appearance: none;-moz-appearance: none; display: block;margin: 0;width: 100%;height: 56px;line-height: 40px;font-size: 40px;border: 1px solid #bbb;}a, a.vsited {font-size: 15px;text-decoration: none;color: #ffffff;background-color: #005ca9;padding: 10px;display: inline-block;border: 1px solid black;border-radius: 10px;text-transform: uppercase;font-weight: bold;}pre {display: block;background-color: #f0f0f0;border: 1px solid black;font-size: 17px;}</style><meta name='apple-mobile-web-app-capable' content='yes'><meta name='viewport' content='user-scalable=no, initial-scale=.5 width=device-width'><meta charset='UTF-8'></head><body><p><a href='/'>Home</a><a href='/wifi'>WiFi config</a><a href='/cfg'>Settings</a><a href='/mppt'>MPPT</a><a href='/sensors'>Sensors</a><hr>";
String html_head_refresh = 
"<html>\
<head>\
<title>Camper Control</title>\
<style>body {font-family: verdana;background-color: white; color: black;font-size: 40px;}\
td,th{font-size: 40px;text-align:left;}\
input[type=checkbox]{padding:2px;transform:scale(3);}\
input[type=password],select,input[type=submit],input[type=text],\
input[type=number]{-webkit-appearance: none;-moz-appearance: none; display: block;margin: 0;width: 100%;height: 56px;line-height: 40px;font-size: 40px;border: 1px solid #bbb;}\
a, a.vsited {font-size: 15px;text-decoration: none;color: #ffffff;background-color: #005ca9;padding: 10px;display: inline-block;border: 1px solid black;border-radius: 10px;text-transform: uppercase;font-weight: bold;}\
pre {display: block;background-color: #f0f0f0;border: 1px solid black;font-size: 17px;}\
</style>\
<meta name='apple-mobile-web-app-capable' content='yes'><meta name='viewport' content='user-scalable=no, initial-scale=.5 width=device-width'>\
<meta charset='UTF-8'>\
<meta http-equiv='refresh' content='5' />\
<style>table, th, td {border: 1px solid black;border-collapse: collapse}</style>\
</head>\
<body><p>\
<a href='/'>Home</a>\
<a href='/wifi'>WiFi config</a>\
<a href='/cfg'>Settings</a>\
<a href='/cfgSpecial'>Special</a>\
<a href='/mppt'>MPPT</a>\
<a href='/sensors'>Sensors</a>\
<hr>";
//"<html><head><title>Camper Control</title><style>body {font-family: verdana;background-color: white; color: black;font-size: 40px;}td,th{font-size: 40px;text-align:left;}input[type=checkbox]{padding:2px;transform:scale(3);}input[type=password],select,input[type=submit],input[type=text],input[type=number]{-webkit-appearance: none;-moz-appearance: none; display: block;margin: 0;width: 100%;height: 56px;line-height: 40px;font-size: 40px;border: 1px solid #bbb;}a, a.vsited {font-size: 15px;text-decoration: none;color: #ffffff;background-color: #005ca9;padding: 10px;display: inline-block;border: 1px solid black;border-radius: 10px;text-transform: uppercase;font-weight: bold;}pre {display: block;background-color: #f0f0f0;border: 1px solid black;font-size: 17px;}</style><meta name='apple-mobile-web-app-capable' content='yes'><meta name='viewport' content='user-scalable=no, initial-scale=.5 width=device-width'><meta charset='UTF-8'><meta http-equiv='refresh' content='5' /></head><body><p><a href='/'>Home</a><a href='/wifi'>WiFi config</a><a href='/cfg'>Settings</a><a href='/mppt'>MPPT</a><a href='/sensors'>Sensors</a><hr>";




void WebServerInit() {
  addLog(LOG_LEVEL_INFO, F("WEB  : Server initializing"));

  // Prepare webserver pages
  Webserver.on("/", handle_root);
  Webserver.on("/mppt", handle_mppt);
  Webserver.on("/sensors", handle_measure);
  Webserver.on("/measure", handle_sensors);
  Webserver.on("/wifi", handle_wificonfig);
  Webserver.on("/savewifi", handle_savewificonfig);
  Webserver.on("/cfg", handle_cfg);
  Webserver.on("/cfgSpecial", handle_cfgSpecial);
  Webserver.on("/savecfg", handle_savecfg);
  Webserver.on("/savecfgSpecial", handle_savecfgSpecial);
  Webserver.on("/json", handle_json);
  Webserver.on("/reset", ResetFactory);
  Webserver.on("/ota", OTA);

  Webserver.onNotFound(handle_notfound);

  Webserver.begin();
}

void handle_wificonfig() {
  String content;
  content = html_head;
  addLog(LOG_LEVEL_INFO, "Scanning for networks...");
  // stop trying to connect to WiFi while searching for networks
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_MODE_AP);
  }
  int n = WiFi.scanNetworks();
  addLog(LOG_LEVEL_INFO, "WEB  : Scan done, found " + String(n) + " networks.");
  if (n == 0) {
    content += "No WiFi networks found.";
  } else {
    content += "<p>" + String(n) + " WiFi networks found.</p>";
    content += "<table border=0>";
    content += "<form action=\"/savewifi\" method=\"get\">\n";
    content += "<tr><td>SSID:</td><td><select name=\"ssid\">\n";
    for (int i = 0; i < n; ++i) {
      content += "<option value=\"";
      String ssid = String(WiFi.SSID(i));
      content += ssid + "\">" + int(i + 1) + ": " + ssid + " ";
      content += " (" + String(WiFi.RSSI(i)) + "dBm) ";
      if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN)
        content += "&#128274;";
      content += "\n";
    }
    content += "</select></td></tr>";
    content += "<tr><td>Key:</td><td><input type=\"password\" name=\"pw\"></td></tr>\n";
    content += "<tr><td>SSID2:</td><td><input name=\"ssid2\"></td></tr>\n";
    content += "<tr><td>Key2:</td><td><input type=\"password\" name=\"pw2\"></td></tr>\n";
    content += "<tr><td>&nbsp;</td><td><input type=\"submit\" value=\"Save\"></td></tr>\n";
    content += "</table>\n";
    content += "</form>";
    content += "</body></html>\n";
  }
  Webserver.send(200, "text/html", content);
}

void handle_savewificonfig() {
  for (int i = 0; i < Webserver.args(); i++) {
    if (Webserver.argName(i) == "ssid")
      Webserver.arg(i).toCharArray(SecuritySettings.WifiSSID, 32);
    if (Webserver.argName(i) == "pw")
      Webserver.arg(i).toCharArray(SecuritySettings.WifiKey, 64);
    if (Webserver.argName(i) == "ssid2")
      Webserver.arg(i).toCharArray(SecuritySettings.WifiSSID2, 32);
    if (Webserver.argName(i) == "pw2")
      Webserver.arg(i).toCharArray(SecuritySettings.WifiKey2, 64);
  }
  addLog(LOG_LEVEL_DEBUG, "WEB  : Config request: SSID: " + String(SecuritySettings.WifiSSID) + " Key: " + String(SecuritySettings.WifiKey));
  addLog(LOG_LEVEL_INFO, "WEB  : Saving settings... " + SaveSettings());

  bool wifi_ok = WifiConnect(3);
  if (wifi_ok) {
    timerAPoff = millis() + 10000L;
  }
  Webserver.sendContent("HTTP/1.1 302\r\nLocation: /\r\n");
}

void formatIP_STR3(const IPAddress& ip, char (&strIP)[20]) 
{
  sprintf_P(strIP, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
}

String formatIP3(const IPAddress& ip)
 {
  char strIP[20];
  formatIP_STR3(ip, strIP);
  return String(strIP);
}




void handle_root() {
  statusLED(true);
  addLog(LOG_LEVEL_DEBUG, F("WEB  : Incoming request for /"));
  if (WiFi.status() == WL_CONNECTED) {
    String content;
    content = html_head;
    content += "Connected to WiFi.<br>IP adres: ";
    content += formatIP3(WiFi.localIP());
    content += "<br>Time: ";
    content += formattedTime();
    Webserver.send(200, "text/html", content);
    if (timerAPoff != 0)
      timerAPoff = millis() + 10000L;
  } else {
    addLog(LOG_LEVEL_INFO, "WEB  : Not connected to WiFi, redirecting to config page");
    Webserver.sendContent("HTTP/1.1 302\r\nLocation: /wifi\r\n");
  }
  statusLED(false);
}
void handle_mppt() {
  statusLED(true);
  addLog(LOG_LEVEL_DEBUG, F("WEB  : Incoming request for /mppt"));
  String content;
  content = html_head_refresh;
  if (MPPT_present) {
    content += "Last MPPT readings:\n";
    content += "<pre>\n";
    content += lastBlockMPPT;
    content += "</pre>";
  } else {
    content += "No MPPT present";
  }
  Webserver.send(200, "text/html", content);
  if (timerAPoff != 0)
    timerAPoff = millis() + 10000L;
  statusLED(false);
}


void handle_measure() {
  statusLED(true);
  addLog(LOG_LEVEL_DEBUG, F("WEB  : Incoming request for /measure"));
  String content;
  content = html_head_refresh;
  
    content += "Last Measurements:\n";
    content += "<pre>\n";
    
content += "<table>";
content += "<tr><td>AD Wandler</td><td>Voltage</td></tr>";
    content += "<tr><td>ADC_0 Ubat</td><td> " + String(AnaValue.ch_0) + "mV</td><td> " + String(AnaValue.Ubatt) + "V</td></tr>";
    content += "<tr><td>ADC_3 Ubal</td><td> " + String(AnaValue.ch_3) + "mV</td><td> " + String(AnaValue.Ubal) + "V</td></tr>";
    content += "<tr><td>ADC_6 Uturb</td><td> " + String(AnaValue.ch_6) + "mV</td><td> " + String(AnaValue.Uturb) + "V</td></tr>";
    content += "<tr><td>ADC_7 Ibatt</td><td> " + String(AnaValue.ch_7) + "mV</td><td> " + String(AnaValue.Ibatt) + "A</td></tr>";
    content += "<tr><td>ADC_4 Iturb</td><td> " + String(AnaValue.ch_4) + "mV</td><td> " + String(AnaValue.Iturb) + "A</td></tr>";
    content += "<tr><td>ADC_5 Uvcc</td><td> " + String(AnaValue.ch_5) + "mV</td><td> " + String(AnaValue.Uvcc) + "V</td></tr>";
    content += "</table>";
    content += "</pre>";
    /*
content += "Last Measurements:\n";
    content += "<pre>\n";
    content += "ADC_0 : " + String(AnaValue.ch_0) + "mV<br>";
    content += "ADC_3 : " + String(AnaValue.ch_3) + "mV<br>";
    content += "ADC_6 : " + String(AnaValue.ch_6) + "mV<br>";
    content += "ADC_7 : " + String(AnaValue.ch_7) + "mV<br>";
    content += "ADC_4 : " + String(AnaValue.ch_4) + "mV<br>";
    content += "ADC_5 : " + String(AnaValue.ch_5) + "mV<br>";
    content += "</pre>";
    */
  Webserver.send(200, "text/html", content);
  if (timerAPoff != 0)
    timerAPoff = millis() + 10000L;
  statusLED(false);
}

void handle_sensors() {
  statusLED(true);
  addLog(LOG_LEVEL_DEBUG, F("WEB  : Incoming request for /sensors"));
  String content;
  content = html_head;
  content += "<h2>Temperature sensors:</h2>";
  for (int i = 0; i < 10; i++) {
    if (readings.temp[i] != -127) {
      content += "Sensor " + String(i) + ": " + String(readings.temp[i]) + "&deg;C<br>";
    }
  }
  content += "<h2>GPS:</h2>";

  content += "gps:";
  content += "fix:" + String(readings.GPS_fix) + "<br>";
  content += "date:" + String(readings.GPS_date) + "<br>";
  content += "time:" + String(readings.GPS_time) + "<br>";
  content += "lat:" + String(readings.GPS_lat) + "<br>";
  //  content += "lat_abs:" + String(readings.GPS_lat_abs) + "<br>";
  content += "lon:" + String(readings.GPS_lon) + "<br>";
  //  content += "lon_abs:" + String(readings.GPS_lon_abs) + "<br>";
  content += "sat:" + String(readings.GPS_sat) + "<br>";
  content += "hdop:" + String(readings.GPS_dop) + "<br>";
  content += "geohash:" + String(readings.GPS_geohash) + "<br>";
  content += "<br>";

  Webserver.send(200, "text/html", content);
  if (timerAPoff != 0)
    timerAPoff = millis() + 10000L;
  statusLED(false);
}

void handle_cfg() {
  statusLED(true);
  addLog(LOG_LEVEL_DEBUG, F("WEB  : Incoming request for /cfg"));
  String content;
  content = html_head;
  content += "<form action=\"/savecfg\" method=\"get\">";
  content += "<table>";
  content += "<tr><th>Influxdb settings</th><th></th></tr>";

  content += "<tr><td>Write data</td><td><input type=\"checkbox\"";
  if (Settings.upload_influx)
    content += " checked";
  content += " name=\"idb_enabled\" value=\"1\"></td></tr>";

  content += "<tr><td>Host</td><td><input type=\"text\" name=\"idb_host\" value=\"" + String(Settings.influx_host) + "\"></td></tr>";
  content += "<tr><td>Port number</td><td><input type=\"number\" name=\"idb_port\" value=\"" + String(Settings.influx_port) + "\"></td></tr>";

  content += "<tr><td>Use SSL</td><td><input type=\"checkbox\"";
  if (Settings.influx_ssl)
    content += " checked";
  content += " name=\"idb_ssl\" value=\"1\"></td></tr>";
  content += "<tr><td>Token</td><td><input type=\"text\" name=\"idb_token\" value=\"" + String(Settings.influx_token) + "\"></td></tr>";
  content += "<tr><td>Organisation name</td><td><input type=\"text\" name=\"idb_org\" value=\"" + String(Settings.influx_org) + "\"></td></tr>";
  content += "<tr><td>Bucket name</td><td><input type=\"text\" name=\"idb_bucket\" value=\"" + String(Settings.influx_bucket) + "\"></td></tr>";
  content += "<tr><td>&nbsp;</td><td></td></tr>";
  content += "<tr><th>What to write to influxdb:</th><th></th></tr>";

content += "<tr><td>MPPT</td><td><input type=\"checkbox\"";
  if (Settings.influx_write_mppt)
    content += " checked";
content += " name=\"idb_mppt\" value=\"1\"></td></tr>";
content += "<tr><td>GPS</td><td><input type=\"checkbox\"";
  if (Settings.influx_write_gps)
    content += " checked";
content += " name=\"idb_gps\" value=\"1\"></td></tr>";

  /*  
  content += "<tr><td>Temperatures</td><td><input type=\"checkbox\"";
  if (Settings.influx_write_temp)
    content += " checked";
  content += " name=\"idb_temp\" value=\"1\"></td></tr>";

  content += "<tr><td>GPS coords</td><td><input type=\"checkbox\"";
  if (Settings.influx_write_coords)
    content += " checked";
  content += " name=\"idb_coords\" value=\"1\"></td></tr>";

  content += "<tr><td>Geohash</td><td><input type=\"checkbox\"";
  if (Settings.influx_write_geohash)
    content += " checked";
  content += " name=\"idb_geohash\" value=\"1\"></td></tr>";

  content += "<tr><td>Speed / heading</td><td><input type=\"checkbox\"";
  if (Settings.influx_write_speed_heading)
    content += " checked";
  content += " name=\"idb_speed\" value=\"1\"></td></tr>";
*/
  content += "<tr><td>&nbsp;</td><td></td></tr>";
  content += "<tr><th>HTTP GET:</th><th></th></tr>";

  content += "<tr><td>Write data</td><td><input type=\"checkbox\"";
  if (Settings.upload_get)
    content += " checked";
  content += " name=\"get_enabled\" value=\"1\"></td></tr>";

  content += "<tr><td>Host:</td><td><input type=\"text\" name=\"get_host\" value=\"" + String(Settings.upload_get_host) + "\"></td></tr>";
  content += "<tr><td>Port number</td><td><input type=\"number\" name=\"get_port\" value=\"" + String(Settings.upload_get_port) + "\"></td></tr>";

  content += "<tr><td>Use SSL</td><td><input type=\"checkbox\"";
  if (Settings.upload_get_ssl)
    content += " checked";
  content += " name=\"get_ssl\" value=\"1\"></td></tr>";

  content += "<tr><td>&nbsp;</td><td></td></tr>";
  content += "<tr><th>Intervals</th><th></th></tr>";

  content += "<tr><td>Upload interval</td><td><input type=\"number\" name=\"idb_interval\" value=\"" + String(Settings.readings_upload_interval) + "\"></td></tr>";
  content += "<tr><td>GPS upload interval</td><td><input type=\"number\" name=\"gps_interval\" value=\"" + String(Settings.gps_upload_interval) + "\"></td></tr>";

  content += "<tr><td colspan=\"2\"><input type=\"submit\" value=\"Save settings\"></td></tr>";
  content += "</table></form></html>";
  Webserver.send(200, "text/html", content);
  if (timerAPoff != 0)
    timerAPoff = millis() + 10000L;
  statusLED(false);
}

void handle_savecfg() {
  statusLED(true);
  addLog(LOG_LEVEL_DEBUG, F("WEB  : Incoming request for /savecfg"));
  // Set booleans to 0, we don't get variables from unchecked checkboxes.
  Settings.upload_influx = 0;
  Settings.influx_ssl = 0;
  Settings.upload_get = 0;
  Settings.upload_get_ssl = 0;
  Settings.influx_write_geohash = 0;
  Settings.influx_write_coords = 0;
  Settings.influx_write_speed_heading = 0;

  for (int i = 0; i < Webserver.args(); i++) {
    if (Webserver.argName(i) == "get_enabled" && Webserver.arg(i) == "1") {
      Settings.upload_get = 1;
    }
    if (Webserver.argName(i) == "idb_ssl" && Webserver.arg(i) == "1") {
      Settings.influx_ssl = 1;
    }
    if (Webserver.argName(i) == "idb_enabled" && Webserver.arg(i) == "1") {
      Settings.upload_influx = 1;
    }
    if (Webserver.argName(i) == "idb_mppt" && Webserver.arg(i) == "1") {
      Settings.influx_write_mppt = 1;
    }
    if (Webserver.argName(i) == "idb_gps" && Webserver.arg(i) == "1") {
      Settings.influx_write_gps = 1;
    }
      if (Webserver.argName(i) == "idb_temp" && Webserver.arg(i) == "1") {
        Settings.influx_write_temp = 1;
      }
      if (Webserver.argName(i) == "idb_geohash" && Webserver.arg(i) == "1") {
        Settings.influx_write_geohash = 1;
      }
      if (Webserver.argName(i) == "idb_coords" && Webserver.arg(i) == "1") {
        Settings.influx_write_coords = 1;
      }
      if (Webserver.argName(i) == "idb_speed" && Webserver.arg(i) == "1") {
        Settings.influx_write_speed_heading = 1;
      }
      if (Webserver.argName(i) == "get_ssl" && Webserver.arg(i) == "1") {
        Settings.upload_get_ssl = 1;
      }
      if (Webserver.argName(i) == "idb_host") {
        Webserver.arg(i).toCharArray(Settings.influx_host, 96);
      }
      if (Webserver.argName(i) == "idb_token") {
        Webserver.arg(i).toCharArray(Settings.influx_token, 100);
      }
      if (Webserver.argName(i) == "idb_org") {
        Webserver.arg(i).toCharArray(Settings.influx_org, 20);
      }
      if (Webserver.argName(i) == "idb_bucket") {
        Webserver.arg(i).toCharArray(Settings.influx_bucket, 20);
      }
      if (Webserver.argName(i) == "get_host") {
        Webserver.arg(i).toCharArray(Settings.upload_get_host, 32);
      }
      if (Webserver.argName(i) == "idb_port") {
        Settings.influx_port = Webserver.arg(i).toInt();
      }
      if (Webserver.argName(i) == "get_port") {
        Settings.upload_get_port = Webserver.arg(i).toInt();
      }
      if (Webserver.argName(i) == "gps_interval") {
        Settings.gps_upload_interval = Webserver.arg(i).toInt();
      }
      if (Webserver.argName(i) == "idb_interval") {
        Settings.readings_upload_interval = Webserver.arg(i).toInt();
      }
    }

    String content;
    SaveSettings();
    Webserver.sendContent("HTTP/1.1 302\r\nLocation: /cfg\r\n");
    statusLED(false);
  }

  void handle_json() {
    statusLED(true);
    String content;
    content = "{\"mppt\":{";
    content += "\"ytot\":\"" + String(readings.MPPT_ytot) + "\",";
    content += "\"yday\":\"" + String(readings.MPPT_yday) + "\",";
    content += "\"Pmax\":\"" + String(readings.MPPT_Pmax) + "\",";
    content += "\"err\":\"" + String(readings.MPPT_err) + "\",";
    content += "\"state\":\"" + String(readings.MPPT_state) + "\",";
    content += "\"Vbatt\":\"" + String(readings.MPPT_Vbatt) + "\",";
    content += "\"Ibatt\":\"" + String(readings.MPPT_Ibatt) + "\",";
    content += "\"Vpv\":\"" + String(readings.MPPT_Vpv) + "\",";
    content += "\"Ppv\":\"" + String(readings.MPPT_Ppv) + "\",";
    content += "\"PID\":\"" + String(readings.MPPT_PID) + "\",";
    content += "\"serial\":\"" + String(readings.MPPT_serial) + "\"";
    content += "},";
    content += "\"gps\":{";
    content += "\"fix\":\"" + String(readings.GPS_fix) + "\",";
    content += "\"date\":\"" + String(readings.GPS_date) + "\",";
    content += "\"time\":\"" + String(readings.GPS_time) + "\",";
    content += "\"lat\":\"" + String(readings.GPS_lat) + "\",";
    content += "\"lat_abs\":\"" + String(readings.GPS_lat_abs) + "\",";
    content += "\"lon\":\"" + String(readings.GPS_lon) + "\",";
    content += "\"lon_abs\":\"" + String(readings.GPS_lon_abs) + "\",";
    content += "\"speed\":\"" + String(readings.GPS_speed) + "\",";
    content += "\"heading\":\"" + String(readings.GPS_heading) + "\",";
    content += "\"geohash\":\"" + String(readings.GPS_geohash) + "\"";
    content += "},";
    content += "\"temp\":{";
    int tempsensorcount = 0;
    for (int i = 0; i < 10; i++) {
      if (readings.temp[i] != -127) {
        tempsensorcount++;
        //      content += "\"temp" + String(i)+ "\":\"" + String(readings.temp[i]) + "\",";
      }
    }
    if (tempsensorcount > 0) {
      content = content.substring(0, content.length() - 1);
    }
    content += "}";
    content += "}";
    Webserver.send(200, "application/json", content);
    if (timerAPoff != 0)
      timerAPoff = millis() + 10000L;
    statusLED(false);
  }

  void handle_notfound() {
    String message = "WEB  : File not found: ";
    message += Webserver.uri();
    addLog(LOG_LEVEL_DEBUG, message);
    Webserver.send(404, "text/html", "404 - Not Found");
  }



void handle_cfgSpecial() {
  statusLED(true);
  addLog(LOG_LEVEL_DEBUG, F("WEB  : Incoming request for /cfg"));
  String content;
  content = html_head;
  content += "<form action=\"/savecfgSpecial\" method=\"get\">";
  content += "<table>";
  content += "<tr><th>Special settings</th><th></th></tr>";

  content += "<tr><td>Converter ON</td><td><input type=\"checkbox\"";
  if (SpecialSettings.ConverterON)
    content += " checked";
  content += " name=\"converter_on\" value=\"1\"></td></tr>";
  content += "<tr><td>Turbine Stop</td><td><input type=\"checkbox\"";
  if (SpecialSettings.TurbineSTOP)
    content += " checked";
  content += " name=\"turbine_stop\" value=\"1\"></td></tr>";

  content +="<td>&nbsp;</td>";
  content += "<tr><td>Converter Auto</td><td><input type=\"checkbox\"";
  if (SpecialSettings.ConverterAuto)
    content += " checked";
  content += " name=\"converter_auto\" value=\"1\"></td></tr>";
  content += "<tr><td>Voltage Converter ON</td><td><input type=number step=0.01 name=uconverter_on value=" +   String(SpecialSettings.U_ConverterON)  + "></td></tr>";
  content += "<tr><td>Voltage Converter OFF</td><td><input type=number step=0.01 name=uconverter_off value=" + String(SpecialSettings.U_ConverterOFF) + "></td></tr>";
 
  content +="<td>&nbsp;</td>";
  content += "<tr><td>Turbine Auto</td><td><input type=\"checkbox\"";
  if (SpecialSettings.TurbineAuto)
    content += " checked";
  content += " name=\"turbine_auto\" value=\"1\"></td></tr>"; 
  content += "<tr><td>Voltage Turbine STOP</td><td><input type=number step=0.01 name=uturbine_stop value=" +   String(SpecialSettings.U_TurbineSTOP)  + "></td></tr>";
  content += "<tr><td>Voltage Turbine RUN</td><td><input type=number step=0.01 name=uturbine_run value=" +     String(SpecialSettings.U_TurbineRUN)   + "></td></tr>";

  content += "<tr><td colspan=2><button type=button onclick=this.style.background = 'green'></button><input type=submit value=Save settings></td></tr>";
  content += "</table></form></html>";
  Webserver.send(200, "text/html", content);
  if (timerAPoff != 0)
    timerAPoff = millis() + 10000L;
  statusLED(false);
}

void handle_savecfgSpecial() {
  statusLED(true);
  addLog(LOG_LEVEL_DEBUG, F("WEB  : Incoming request for /savecfgSpecial"));
  // Set booleans to 0, we don't get variables from unchecked checkboxes.
 SpecialSettings.TurbineAuto = 0;
  SpecialSettings.TurbineSTOP = 0;
  SpecialSettings.ConverterAuto = 0;
  SpecialSettings.U_ConverterON = 0;
  SpecialSettings.U_ConverterOFF = 0;
  

  for (int i = 0; i < Webserver.args(); i++) {
    if (Webserver.argName(i) == "converter_on" && Webserver.arg(i) == "1") {
      SpecialSettings.ConverterON = 1;
    }
    if (Webserver.argName(i) == "turbine_stop" && Webserver.arg(i) == "1") {
      SpecialSettings.TurbineSTOP = 1;
    }
    if (Webserver.argName(i) == "turbine_auto" && Webserver.arg(i) == "1") {
      SpecialSettings.TurbineAuto = 1;
    }
    if (Webserver.argName(i) == "converter_auto" && Webserver.arg(i) == "1") {
      SpecialSettings.ConverterAuto = 1;
    }
    if (Webserver.argName(i) == "uconverter_on") {
        SpecialSettings.U_ConverterON = Webserver.arg(i).toFloat();
     }
    
    if (Webserver.argName(i) == "uconverter_off") {
        SpecialSettings.U_ConverterOFF = Webserver.arg(i).toFloat();
     }
    if (Webserver.argName(i) == "uturbine_stop") {
        SpecialSettings.U_TurbineSTOP = Webserver.arg(i).toFloat();
     }
    if (Webserver.argName(i) == "uturbine_run") {
        SpecialSettings.U_TurbineRUN = Webserver.arg(i).toFloat();
     }
    }

   // String content;
    SaveSettings();
    Webserver.sendContent("HTTP/1.1 302\r\nLocation: /cfgSpecial\r\n");
    statusLED(false);
  }
