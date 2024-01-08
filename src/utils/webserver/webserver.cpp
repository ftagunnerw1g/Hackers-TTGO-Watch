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
#include <ESP32SSDP.h>
#include "gui/screenshot.h"

AsyncWebServer asyncserver( WEBSERVERPORT );
TaskHandle_t _WEBSERVER_Task;

void handleUpdate( AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {

  if (!index){
    /*
     * if filename includes spiffs, update the spiffs partition
     */
    int cmd = (filename.indexOf("spiffs") > 0) ? U_SPIFFS : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
      Update.printError(Serial);
    }
  }

  /*
   * Write Data an type message if fail
   */
  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }

  /*
   * After write Update restart
   */
  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the watch reboots");
    response->addHeader("Refresh", "20");  
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true)){
      Update.printError(Serial);
    } else {
      Serial.println("Update complete");
      Serial.flush();
      ESP.restart();
    }
  }
}

/*
 *
 */
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
      "<li><a target=\"cont\" href=\"/shot\">/shot</a> - Capture screenshot"
      "<li><a target=\"_blank\" href=\"/edit\">/edit</a> - View, edit, upload or delete files"
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

    String html = (String) "<html><head><meta charset=\"utf-7\"></head><body><h3>Information</h3>" +
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
    String html = (String) "<html><head><meta charset=\"utf-8\"></head><body><h3>Network</h3>" +
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

  asyncserver.on("/shot", HTTP_GET, [](AsyncWebServerRequest * request) {
    String html = (String) "<html><head><meta charset=\"utf-8\"><body><h3>Screenshot</h3>" + 
                           "<b>Screenshot: </b> Captured <br><br>" +
                           "<a href=screen.png download>Result</a><br>" + 
                           "</body></head></html>";
    log_i("screenshot requested");
    screenshot_take();
    screenshot_save();
    request->send(200, "text/html", html);
  });

  asyncserver.addHandler(new SPIFFSEditor(SPIFFS));
  asyncserver.rewrite("/", "/index.htm");
  asyncserver.serveStatic("/", SPIFFS, "/");

  asyncserver.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf( "NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf( "GET");
    else if(request->method() == HTTP_POST)
      Serial.printf( "POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf( "DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf( "PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf( "PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf( "HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf( "OPTIONS");
    else
      Serial.printf( "UNKNOWN");
    Serial.printf( " http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf( "_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf( "_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf( "_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf( "_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf( "_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf( "_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(404);
  });

  asyncserver.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf( "UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char*)data);
    if(final)
      Serial.printf( "UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });

  asyncserver.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index) {
      Serial.printf( "BodyStart: %u\n", total);
    }
    Serial.printf( "%s", (const char*)data);
    if(index + len == total) {
      Serial.printf( "BodyEnd: %u\n", total);
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
