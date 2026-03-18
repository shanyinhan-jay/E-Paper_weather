#include "WebHandler.h"
#include <Arduino.h>
#include <Update.h>
#include "Local_EPD_4in2.h"
#include "GUI_Paint.h"

void handleRoot() {
  String html = INDEX_HTML_TEMPLATE;
  Serial.print("HTML Length: "); Serial.println(html.length());
  
  // Replace placeholders manually since server.send() doesn't support template processor directly like ESPAsyncWebServer
  // A more efficient way would be to stream it, but for simplicity we replace strings
  if (digitalRead(MODE_PIN) == HIGH) {
      String batHtml = "<span>Battery: <strong style='color: var(--primary);'>" + String(getBatteryVoltage(), 2) + "V</strong></span>";
      html.replace("%BATTERY_INFO%", batHtml);
  } else {
      html.replace("%BATTERY_INFO%", "");
  }
  
  html.replace("%BUILD_DATE%", String(build_date));
  html.replace("%BUILD_TIME%", String(build_time));
  html.replace("%CSS%", COMMON_CSS);
  html.replace("%WIFI_SSID%", String(config.wifi_ssid));
  html.replace("%WIFI_PASS%", String(config.wifi_pass));
  html.replace("%DEVICE_NAME%", String(config.device_name));
  html.replace("%MQTT_SERVER%", String(config.mqtt_server));
  html.replace("%MQTT_PORT%", String(config.mqtt_port));
  html.replace("%MQTT_USER%", String(config.mqtt_user));
  html.replace("%MQTT_PASS%", String(config.mqtt_pass));
  html.replace("%MQTT_TOPIC%", String(config.mqtt_topic));
  html.replace("%MQTT_WEATHER%", String(config.mqtt_weather_topic));
  html.replace("%MQTT_BATTERY%", String(config.mqtt_battery_topic));
  html.replace("%MQTT_DATE%", String(config.mqtt_date_topic));
  html.replace("%MQTT_ENV%", String(config.mqtt_env_topic));
  html.replace("%MQTT_CALENDAR%", String(config.mqtt_calendar_topic));
  html.replace("%MQTT_SHIFT%", String(config.mqtt_shift_topic));
  html.replace("%MQTT_AQI%", String(config.mqtt_air_quality_topic));
  html.replace("%MQTT_UNIFIED%", String(config.mqtt_unified_topic));
  html.replace("%MQTT_REQUEST%", String(config.mqtt_request_topic));
  html.replace("%NTP_SERVER%", String(config.ntp_server));
  html.replace("%NTP_SERVER_2%", String(config.ntp_server_2));
  html.replace("%FULL_REFRESH%", String(config.full_refresh_period));
  html.replace("%REQUEST_INTERVAL%", String(config.request_interval));
  html.replace("%SLEEP_DELAY%", String(config.sleep_delay));
  html.replace("%CONFIG_TIMEOUT%", String(config.config_timeout));
  html.replace("%DAY_START%", String(config.day_start_hour));
  html.replace("%DAY_END%", String(config.day_end_hour));
  
  // Static IP
  html.replace("%USE_STATIC_IP%", config.use_static_ip ? "checked" : "");
  html.replace("%STATIC_IP%", String(config.static_ip));
  html.replace("%STATIC_GW%", String(config.static_gw));
  html.replace("%STATIC_MASK%", String(config.static_mask));
  html.replace("%STATIC_DNS%", String(config.static_dns));

  // Handle Radio Button State
  if (config.ui_mode == 1) {
      html.replace("%UI_MODE_0%", "");
      html.replace("%UI_MODE_1%", "checked");
  } else {
      html.replace("%UI_MODE_0%", "checked");
      html.replace("%UI_MODE_1%", "");
  }
  
  html.replace("%ADC_PIN%", String(config.adc_pin));
  html.replace("%ADC_RATIO%", String(config.adc_ratio, 2));
  html.replace("%LOW_BATTERY_THRESHOLD%", String(config.low_battery_threshold, 2));

  if (config.invert_display) {
      html.replace("%INVERT_0%", "");
      html.replace("%INVERT_1%", "checked");
  } else {
      html.replace("%INVERT_0%", "checked");
      html.replace("%INVERT_1%", "");
  }
  
  server.send(200, "text/html", html);
}

void handleMqttConfig() {
  String html = MQTT_CONFIG_HTML_TEMPLATE;
  html.replace("%CSS%", COMMON_CSS);
  
  html.replace("%MQTT_TOPIC%", String(config.mqtt_topic));
  html.replace("%MQTT_WEATHER%", String(config.mqtt_weather_topic));
  html.replace("%MQTT_BATTERY%", String(config.mqtt_battery_topic));
  html.replace("%MQTT_DATE%", String(config.mqtt_date_topic));
  html.replace("%MQTT_ENV%", String(config.mqtt_env_topic));
  html.replace("%MQTT_CALENDAR%", String(config.mqtt_calendar_topic));
  html.replace("%MQTT_SHIFT%", String(config.mqtt_shift_topic));
  html.replace("%MQTT_AQI%", String(config.mqtt_air_quality_topic));
  html.replace("%MQTT_UNIFIED%", String(config.mqtt_unified_topic));
  html.replace("%MQTT_REQUEST%", String(config.mqtt_request_topic));
  
  server.send(200, "text/html", html);
}

void handleFileManager() {
  String html = FILE_MANAGER_HTML_TEMPLATE;
  html.replace("%CSS%", COMMON_CSS);
  String fileList = "";
  
  File root = LittleFS.open("/icons");
  if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while(file){
          String fileName = String(file.name());
          fileList += "<li>" + fileName + " (" + String(file.size()) + " bytes) ";
          fileList += "<a href='/delete?file=" + fileName + "' onclick='return confirm(\"Delete this file?\");'>[Delete]</a></li>";
          file = root.openNextFile();
      }
  } else {
      fileList = "<li>No files found or directory missing</li>";
  }
  
  html.replace("%FILE_LIST%", fileList);
  server.send(200, "text/html", html);
}

void handleUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    if (!filename.startsWith("/icons/")) filename = "/icons" + filename; // Force icons folder
    
    Serial.print("Upload start: "); Serial.println(filename);
    // Ensure directory exists
    if (!LittleFS.exists("/icons")) LittleFS.mkdir("/icons");
    
    File f = LittleFS.open(filename, "w");
    if (!f) Serial.println("Failed to open file for writing");
    f.close();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    if (!filename.startsWith("/icons/")) filename = "/icons" + filename;
    
    File f = LittleFS.open(filename, "a");
    if (f) {
        f.write(upload.buf, upload.currentSize);
        f.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.print("Upload Size: "); Serial.println(upload.totalSize);
    server.sendHeader("Location", "/files");
    server.send(303);
  }
}

void handleUpdate() {
  String html = UPDATE_HTML_TEMPLATE;
  html.replace("%CSS%", COMMON_CSS);
  server.send(200, "text/html", html);
}

void handleUpdateFirmware() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // start with max available size
      Serial.print("Update.begin error: ");
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    /* flashing firmware to ESP */
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.print("Update.write error: ");
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { // true to set the size to the current progress
      Serial.printf("Update Success: %u bytes. Rebooting...\n", upload.totalSize);
    } else {
      Serial.print("Update.end error: ");
      Update.printError(Serial);
    }
  }
}

void handleDelete() {
    if (server.hasArg("file")) {
        String filename = server.arg("file");
        if (!filename.startsWith("/")) filename = "/" + filename;
        if (!filename.startsWith("/icons/")) filename = "/icons" + filename;
        
        if (LittleFS.exists(filename)) {
            LittleFS.remove(filename);
            Serial.print("Deleted: "); Serial.println(filename);
        }
    }
  server.sendHeader("Location", "/files");
  server.send(303);
}

void handleReboot() {
  server.send(200, "text/html", "<html><body><h1>Rebooting...</h1><p>Please wait while the device restarts.</p></body></html>");
  delay(500);
  ESP.restart();
}

void handleSaveConfig() {
  if (server.hasArg("wifi_ssid")) strlcpy(config.wifi_ssid, server.arg("wifi_ssid").c_str(), sizeof(config.wifi_ssid));
  if (server.hasArg("wifi_pass")) strlcpy(config.wifi_pass, server.arg("wifi_pass").c_str(), sizeof(config.wifi_pass));
  if (server.hasArg("device_name")) strlcpy(config.device_name, server.arg("device_name").c_str(), sizeof(config.device_name));
  if (server.hasArg("mqtt_server")) strlcpy(config.mqtt_server, server.arg("mqtt_server").c_str(), sizeof(config.mqtt_server));
  if (server.hasArg("mqtt_port")) config.mqtt_port = server.arg("mqtt_port").toInt();
  if (server.hasArg("mqtt_user")) strlcpy(config.mqtt_user, server.arg("mqtt_user").c_str(), sizeof(config.mqtt_user));
  if (server.hasArg("mqtt_pass")) strlcpy(config.mqtt_pass, server.arg("mqtt_pass").c_str(), sizeof(config.mqtt_pass));
  if (server.hasArg("mqtt_topic")) strlcpy(config.mqtt_topic, server.arg("mqtt_topic").c_str(), sizeof(config.mqtt_topic));
  if (server.hasArg("mqtt_weather_topic")) strlcpy(config.mqtt_weather_topic, server.arg("mqtt_weather_topic").c_str(), sizeof(config.mqtt_weather_topic));
  if (server.hasArg("mqtt_date_topic")) strlcpy(config.mqtt_date_topic, server.arg("mqtt_date_topic").c_str(), sizeof(config.mqtt_date_topic));
  if (server.hasArg("mqtt_env_topic")) strlcpy(config.mqtt_env_topic, server.arg("mqtt_env_topic").c_str(), sizeof(config.mqtt_env_topic));
  if (server.hasArg("mqtt_calendar_topic")) strlcpy(config.mqtt_calendar_topic, server.arg("mqtt_calendar_topic").c_str(), sizeof(config.mqtt_calendar_topic));
  if (server.hasArg("mqtt_shift_topic")) strlcpy(config.mqtt_shift_topic, server.arg("mqtt_shift_topic").c_str(), sizeof(config.mqtt_shift_topic));
  if (server.hasArg("mqtt_air_quality_topic")) strlcpy(config.mqtt_air_quality_topic, server.arg("mqtt_air_quality_topic").c_str(), sizeof(config.mqtt_air_quality_topic));
  if (server.hasArg("mqtt_battery_topic")) strlcpy(config.mqtt_battery_topic, server.arg("mqtt_battery_topic").c_str(), sizeof(config.mqtt_battery_topic));
  if (server.hasArg("mqtt_unified_topic")) strlcpy(config.mqtt_unified_topic, server.arg("mqtt_unified_topic").c_str(), sizeof(config.mqtt_unified_topic));
  if (server.hasArg("mqtt_request_topic")) strlcpy(config.mqtt_request_topic, server.arg("mqtt_request_topic").c_str(), sizeof(config.mqtt_request_topic));
  if (server.hasArg("ntp_server")) strlcpy(config.ntp_server, server.arg("ntp_server").c_str(), sizeof(config.ntp_server));
  if (server.hasArg("ntp_server_2")) strlcpy(config.ntp_server_2, server.arg("ntp_server_2").c_str(), sizeof(config.ntp_server_2));
  if (server.hasArg("full_refresh_period")) config.full_refresh_period = server.arg("full_refresh_period").toInt();
  if (server.hasArg("request_interval")) config.request_interval = server.arg("request_interval").toInt();
  if (server.hasArg("sleep_delay")) config.sleep_delay = server.arg("sleep_delay").toInt();
  if (server.hasArg("config_timeout")) config.config_timeout = server.arg("config_timeout").toInt();
  if (server.hasArg("day_start_hour")) config.day_start_hour = server.arg("day_start_hour").toInt();
  if (server.hasArg("day_end_hour")) config.day_end_hour = server.arg("day_end_hour").toInt();
  
  if (server.hasArg("invert_display")) config.invert_display = (server.arg("invert_display") == "1");
  if (server.hasArg("ui_mode")) config.ui_mode = server.arg("ui_mode").toInt();
  if (server.hasArg("adc_pin")) config.adc_pin = server.arg("adc_pin").toInt();
  if (server.hasArg("adc_ratio")) config.adc_ratio = server.arg("adc_ratio").toFloat();
  if (server.hasArg("low_battery_threshold")) config.low_battery_threshold = server.arg("low_battery_threshold").toFloat();

  // Static IP Handling
  if (server.hasArg("use_static_ip")) config.use_static_ip = true;
  else config.use_static_ip = false; // Checkbox unchecked means false

  if (server.hasArg("static_ip")) strlcpy(config.static_ip, server.arg("static_ip").c_str(), sizeof(config.static_ip));
  if (server.hasArg("static_gw")) strlcpy(config.static_gw, server.arg("static_gw").c_str(), sizeof(config.static_gw));
  if (server.hasArg("static_mask")) strlcpy(config.static_mask, server.arg("static_mask").c_str(), sizeof(config.static_mask));
  if (server.hasArg("static_dns")) strlcpy(config.static_dns, server.arg("static_dns").c_str(), sizeof(config.static_dns));

  saveConfig();
  
  server.send(200, "text/plain", "OK");
}

void handleSetPage() {
  if (server.hasArg("page")) {
    String page = server.arg("page");
    if (page == "weather") {
        currentPage = PAGE_WEATHER;
        switchPagePending = true;
        server.sendHeader("Location", "/");
        server.send(303);
    } else if (page == "calendar") {
        currentPage = PAGE_CALENDAR;
        switchPagePending = true;
        server.sendHeader("Location", "/");
        server.send(303);
    } else {
        server.send(400, "text/plain", "Invalid page");
    }
  } else {
    server.send(400, "text/plain", "Missing page argument");
  }
}

void handleSetText() {
  if (server.hasArg("text")) {
    String text = server.arg("text");
    server.send(200, "text/plain", "Displaying: " + text);
    displayMessage(text);
  } else {
    server.send(400, "text/plain", "Missing text argument");
  }
}
