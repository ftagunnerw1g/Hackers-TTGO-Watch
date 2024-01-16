/****************************************************************************
 *   Tu May 22 21:23:51 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "webserver.h"
#include "config.h"

#ifdef NATIVE_64BIT

void asyncwebserver_start(void){ return; };
void asyncwebserver_end(void){ return; }

#else

#include <WiFi.h>
#include <WiFiClient.h>
#include <Update.h>
#include <SPIFFS.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include "gui/screenshot.h"
#include "hardware/pmu.h"

AsyncWebServer asyncserver( WEBSERVERPORT );
TaskHandle_t _WEBSERVER_Task;

void asyncwebserver_start(void){

  asyncserver.on("/index.htm", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = (String) "<!DOCTYPE html>"
      "<html>"
      "<frameset cols=\"300, *\">"
      "<frame src=\"/nav.htm\" name=\"nav\">"
      "<frame name=\"cont\">"
      "</frameset>"
      "</html>";
    request->send(200, "text/html", html);
  });

  asyncserver.on("/nav.htm", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = (String) "<!DOCTYPE html>"
      "<html><head>"
      "<meta http-equiv='Content-type' content='text/html; charset=utf-8'>"
      "<title>Web Interface</title>"
      "</head><body>"
      "<h1>TTGo Watch</h1>"
      "<p>URL:"
      "<ul>"
      "<li><a target=\"cont\" href=\"/info\">/info</a> - Device information"
      "<li><a target=\"cont\" href=\"/network\">/network</a> - Network information"
      "<li><a target=\"cont\" href=\"/power\">/power</a> - Battery information"
      "<li><a target=\"cont\" href=\"/shot\">/shot</a> - Capture screenshot"
      "<li><a target=\"cont\" href=\"/files\">/files</a> - Config files"
      "</ul>"
      "</body></html>";
    request->send(200, "text/html", html);
  });

  asyncserver.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    setenv("TZ", "UTC", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    String html = (String) "<html><head><meta charset=\"utf-7\"><body><h3>Information</h3>" +

#if defined ( LILYGO_WATCH_2020_V1 )
                  "<b><u>Hardware</u></b><br>" +
                  "LilyGo Watch 2020 V1<br><br>" +
#elif defined( LILYGO_WATCH_2020_V2  )
                  "<b><u>Hardware</u></b><br>" +
                  "LilyGo Watch 2020 V2<br><br>" +
#elif defined( LILYGO_WATCH_2020_V2  )
                  "<b><u>Hardware</u></b><br>" +
                  "LilyGo Watch 2020 V3<br><br>" +
#endif

                  "<b><u>Time</u></b><br>" +
                   "<b>UTC: </b>" + strftime_buf + "<br><br>" +  
                  "<b><u>Memory</u></b><br>" +
                  "<b>Heap size: </b>" + ESP.getHeapSize() + "<br>" +
                  "<b>Heap free: </b>" + ESP.getFreeHeap() + "<br>" +
                  "<b>Heap free min: </b>" + ESP.getMinFreeHeap() + "<br>" +
                  "<b>Heap size: </b>" + ESP.getHeapSize() + "<br>" +
                  "<b>Psram size: </b>" + ESP.getPsramSize() + "<br>" +
                  "<b>Psram free: </b>" + ESP.getFreePsram() + "<br>" +

                  "<br><b><u>System</u></b><br>" +
                  "\t<b>Uptime: </b>" + millis() / 1000 + "<br>" +
                  "<br><b><u>Chip</u></b>" +
                  "<br><b>SdkVersion: </b>" + String(ESP.getSdkVersion()) + "<br>" +
                  "<b>CpuFreq: </b>" + String(ESP.getCpuFreqMHz()) + " MHz<br>" +

                  "<br><b><u>Flash</u></b><br>" +
                  "<b>FlashChipMode: </b>" + ESP.getFlashChipMode() + "<br>" +
                  "<b>FlashChipSize (SDK): </b>" + ESP.getFlashChipSize() + "<br>" +

                  "<br><b><u>Filesystem</u></b><br>" +
                  "<b>Total size: </b>" + SPIFFS.totalBytes() + "<br>" +
                  "<b>Used size: </b>" + SPIFFS.usedBytes() + "<br>" +
 
                  "<br>";
    request->send(200, "text/html", html);
  });

  asyncserver.on("/network", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = (String) "<html><head><meta charset=\"utf-8\"><body><h3>Network</h3>" +
                  "<b>IP Addr: </b>" + WiFi.localIP().toString() + "<br>" +
                  "<b>MAC: </b>" + WiFi.macAddress() + "<br>" +
                  "<b>SNMask: </b>" + WiFi.subnetMask().toString() + "<br>" +
                  "<b>GW IP: </b>" + WiFi.gatewayIP().toString() + "<br>" +
                  "<b>DNS 1: </b>" + WiFi.dnsIP(0).toString() + "<br>" +
                  "<b>DNS 2: </b>" + WiFi.dnsIP(1).toString() + "<br>" +
                  "<b>RSSI: </b>" + String(WiFi.RSSI()) + "dB<br>" +
                  "<b>Hostname: </b>" + WiFi.getHostname() + "<br>" +
                  "<b>SSID: </b>" + WiFi.SSID() + "<br>" +
                  "</body></head></html>";
    request->send(200, "text/html", html);
  });

  asyncserver.on("/power", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = (String) "<html><head><meta charset=\"utf-8\"><body><h3>Power</h3>" +
                  "<b>Designed capacity: </b>" + pmu_get_designed_battery_cap() + "mAh<br>" +
                  "<b>Battery voltage: </b>" + pmu_get_battery_voltage() / 1000 + "V<br>" +
                  "<b>Charge current: </b>" + pmu_get_battery_charge_current() + "mA<br>" +
                  "<b>Discharge current: </b>" + pmu_get_battery_discharge_current() + "mA<br>" +
                  "<b>VBUS voltage: </b>" + pmu_get_vbus_voltage() / 1000 + "V<br>" +
                  "<b>Battery percent: </b>" + pmu_get_battery_percent() + "%<br>" +
                  "</body></head></html>";
    request->send(200, "text/html", html);
  });

  asyncserver.on("/files", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = (String) "<html><head><meta charset=\"utf-8\"><body><h3>Config</h3>" +
                           "<a href=\"/display.json\">/display.json</a> - Display configuration<br>"
#if defined( LILYGO_WATCH_HAS_GPS ) 
                           "<a href=\"/gpsctl.json\">/gpsctl.json</a> - GPS configuration<br>"
#endif
                           "<a href=\"/motor.json\">/motor.json</a> - Motor configuration<br>"
#if defined( LILYGO_WATCH_HAS_GPS ) 
                           "<a href=\"/osmmap.json\">/osmmap.json</a> - OSMMAP configuration<br>"
#endif
                           "<a href=\"/timesync.json\">/timesync.json</a> - Time sync configuration<br>"
                           "<a href=\"/wificfg.json\">/wificfg.json</a> - WiFi configuration<br>"
                           "</body></head></html>";
    request->send(200, "text/html", html);
  });

  asyncserver.on("/shot", HTTP_GET, [](AsyncWebServerRequest * request) {
    String html = (String) "<html><head><meta charset=\"utf-8\"><body><h3>Screenshot</h3>" + 
			   "<img src=screen.png><br>"
                           "</body></head></html>";
    log_d("screenshot requested");
    screenshot_take();
    screenshot_save();
    request->send(200, "text/html", html);
  });

  asyncserver.addHandler(new SPIFFSEditor(SPIFFS));
  asyncserver.rewrite("/", "/index.htm");
  asyncserver.serveStatic("/", SPIFFS, "/");

  asyncserver.onNotFound([](AsyncWebServerRequest *request){
    log_d( "NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      log_d( "GET");
    else if(request->method() == HTTP_POST)
      log_d( "POST");
    else if(request->method() == HTTP_DELETE)
      log_d( "DELETE");
    else if(request->method() == HTTP_PUT)
      log_d( "PUT");
    else if(request->method() == HTTP_PATCH)
      log_d( "PATCH");
    else if(request->method() == HTTP_HEAD)
      log_d( "HEAD");
    else if(request->method() == HTTP_OPTIONS)
      log_d( "OPTIONS");
    else
      log_d( "UNKNOWN");
      log_d( " http://%s%s", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      log_d( "_CONTENT_TYPE: %s", request->contentType().c_str());
      log_d( "_CONTENT_LENGTH: %u", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      log_d( "_HEADER[%s]: %s", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        log_d( "_FILE[%s]: %s, size: %u", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        log_d( "_POST[%s]: %s", p->name().c_str(), p->value().c_str());
      } else {
        log_d( "_GET[%s]: %s", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(404);
  });

  asyncserver.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
  File file;
  char upload_filename[128];
  snprintf(upload_filename, 128, "/%s", filename); 
  if(!index) {
      log_d( "UploadStart: %s", filename.c_str());
      file = SPIFFS.open(upload_filename, FILE_WRITE);
    }
    if(len) {
      log_i("writing data to file %s", filename.c_str());
      log_d("%s", (const char*)data);
      file.print((const char*)data);
    }
    if(final) {
      log_d( "UploadEnd: %s (%u)", upload_filename, index+len);
      file.close();
      request->send(201);
    }
  });

  asyncserver.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index) {
      log_d( "BodyStart: %u", total);
    }
    log_d( "%s", (const char*)data);
    if(index + len == total) {
      log_d( "BodyEnd: %u", total);
    }
  });

  asyncserver.begin();
  log_i("enable webserver"); 
}

void asyncwebserver_end(void) {
  asyncserver.end();
  log_i("disable webserver");
}
#endif 
