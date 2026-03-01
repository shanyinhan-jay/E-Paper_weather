#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "DEV_Config.h" // For config struct
#include "AppConfig.h"

// Forward declaration
extern Config config;
extern WebServer server;extern const char* build_date;
extern const char* build_time;
extern void saveConfig();
extern void handleMqttConfig();
extern void displayMessage(String text);
extern void displayWeatherDashboard(bool partial_update);
extern void displayCalendarPage(bool partial_update);
extern bool switchPagePending;
extern Page currentPage;

// Common CSS for all pages
const char COMMON_CSS[] PROGMEM = R"css(
<style>
  :root {
    --primary: #2563eb;
    --primary-hover: #1d4ed8;
    --danger: #dc2626;
    --danger-hover: #b91c1c;
    --bg: #f3f4f6;
    --card-bg: #ffffff;
    --text: #1f2937;
    --text-light: #6b7280;
    --border: #e5e7eb;
  }
  body {
    font-family: system-ui, -apple-system, sans-serif;
    background-color: var(--bg);
    color: var(--text);
    margin: 0;
    padding: 20px;
    line-height: 1.5;
  }
  .container {
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background: var(--card-bg);
    border-radius: 8px;
    box-shadow: 0 1px 3px rgba(0,0,0,0.1);
    padding: 20px;
    margin-bottom: 20px;
  }
  h1 { font-size: 1.5rem; margin-bottom: 1rem; color: var(--text); }
  h2 { font-size: 1.25rem; margin-top: 0; margin-bottom: 1rem; padding-bottom: 0.5rem; border-bottom: 1px solid var(--border); }
  h3 { font-size: 1rem; margin-top: 1rem; margin-bottom: 0.5rem; color: var(--text-light); text-transform: uppercase; letter-spacing: 0.05em; font-weight: 600; }
  
  input[type=text], input[type=number], input[type=password], input[type=file] {
    width: 100%;
    padding: 10px;
    margin: 5px 0 15px;
    border: 1px solid var(--border);
    border-radius: 6px;
    box-sizing: border-box;
    font-size: 14px;
    transition: border-color 0.2s;
  }
  input[type=text]:focus, input[type=number]:focus, input[type=password]:focus {
    outline: none;
    border-color: var(--primary);
    box-shadow: 0 0 0 3px rgba(37,99,235,0.1);
  }
  
  button, input[type=submit] {
    background-color: var(--primary);
    color: white;
    border: none;
    padding: 10px 20px;
    border-radius: 6px;
    cursor: pointer;
    width: 100%;
    font-size: 14px;
    font-weight: 500;
    transition: background-color 0.2s;
    margin-top: 5px;
  }
  button:hover, input[type=submit]:hover { background-color: var(--primary-hover); }
  
  .btn-danger { background-color: var(--danger) !important; }
  .btn-danger:hover { background-color: var(--danger-hover) !important; }
  
  .btn-group { display: flex; gap: 10px; }
  .btn-group button { width: 100%; margin: 0; }
  
  .grid-2-col { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
  .grid-2-col input { margin: 0 !important; }
  @media (max-width: 480px) { .grid-2-col { grid-template-columns: 1fr; } }
  
  a { color: var(--primary); text-decoration: none; }
  a:hover { text-decoration: underline; }
  
  .footer { text-align: center; color: var(--text-light); font-size: 0.875rem; margin-top: 40px; }
  ul { list-style: none; padding: 0; }
  li { padding: 10px 0; border-bottom: 1px solid var(--border); display: flex; justify-content: space-between; align-items: center; }
  li:last-child { border-bottom: none; }
  .badge { background: #e5e7eb; padding: 2px 6px; border-radius: 4px; font-size: 12px; }
</style>
)css";

// HTML Templates
const char INDEX_HTML_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>EPD Control</title>
  %CSS%
</head>
<body>
  <div class="container">
    <div style="text-align:center; margin-bottom:30px;">
      <h1>E-Paper Control</h1>
    </div>

    <div class="card">
      <h2>Display Control</h2>
      <form action='/setPage' method='POST'>
        <h3>Page Selection</h3>
        <div class="btn-group">
          <button type='submit' name='page' value='weather'>Weather Dashboard</button>
          <button type='submit' name='page' value='calendar'>Calendar View</button>
        </div>
      </form>
      
      <div style="margin-top: 20px;">
        <form action='/setText' method='POST'>
          <h3>Direct Text</h3>
          <input type='text' name='text' placeholder='Enter text to display...'>
          <input type='submit' value='Display Text'>
        </form>
      </div>
    </div>
    
    <div class="card">
      <h2>Configuration</h2>
      <form action='/saveConfig' method='POST'>
        <h3>Display Mode</h3>
        <div class="grid-2-col">
            <label style="display:flex;align-items:center;gap:5px;cursor:pointer;">
                <input type="radio" name="ui_mode" value="0" %UI_MODE_0% style="width:auto;margin:0;"> TIME Mode
            </label>
            <label style="display:flex;align-items:center;gap:5px;cursor:pointer;">
                <input type="radio" name="ui_mode" value="1" %UI_MODE_1% style="width:auto;margin:0;"> DATE Mode
            </label>
        </div>

        <div class="grid-2-col">
            <label style="display:flex;align-items:center;gap:5px;cursor:pointer;">
                <input type="radio" name="invert_display" value="0" %INVERT_0% style="width:auto;margin:0;"> White Background
            </label>
            <label style="display:flex;align-items:center;gap:5px;cursor:pointer;">
                <input type="radio" name="invert_display" value="1" %INVERT_1% style="width:auto;margin:0;"> Black Background
            </label>
        </div>

        <h3>Pins</h3>
        <div class="grid-2-col">
            <div>
                <label>DONE Pin (GPIO)</label>
                <input type='number' name='done_pin' value='%DONE_PIN%' placeholder="e.g. 33">
            </div>
        </div>

        <h3>WiFi</h3>
        <div class="grid-2-col">
            <input type='text' name='wifi_ssid' value='%WIFI_SSID%' placeholder="SSID">
            <input type='password' name='wifi_pass' value='%WIFI_PASS%' placeholder="Password">
        </div>

        <h3>NTP Servers</h3>
        <div class="grid-2-col">
            <input type='text' name='ntp_server' value='%NTP_SERVER%' placeholder="Primary NTP Server (e.g. ntp.aliyun.com)">
            <input type='text' name='ntp_server_2' value='%NTP_SERVER_2%' placeholder="Secondary NTP Server">
        </div>

        <h3>Static IP (Optional)</h3>
        <label style="display:flex;align-items:center;gap:5px;cursor:pointer;margin-bottom:10px;">
             <input type="checkbox" name="use_static_ip" value="1" %USE_STATIC_IP% style="width:auto;margin:0;"> Enable Static IP
        </label>
        <div class="grid-2-col">
            <input type='text' name='static_ip' value='%STATIC_IP%' placeholder="IP Address">
            <input type='text' name='static_gw' value='%STATIC_GW%' placeholder="Gateway">
            <input type='text' name='static_mask' value='%STATIC_MASK%' placeholder="Subnet Mask">
            <input type='text' name='static_dns' value='%STATIC_DNS%' placeholder="DNS Server">
        </div>
        
        <h3>Refresh Settings (Minutes)</h3>
        <div class="grid-2-col">
            <div>
                <label>Full Refresh Period</label>
                <input type='number' name='full_refresh_period' value='%FULL_REFRESH%' placeholder="0 = Disabled">
            </div>
            <div>
                <label>Request Interval</label>
                <input type='number' name='request_interval' value='%REQUEST_INTERVAL%' placeholder="0 = Disabled">
            </div>
        </div>

        <h3>MQTT Broker</h3>
        <div class="grid-2-col">
            <input type='text' name='mqtt_server' value='%MQTT_SERVER%' placeholder="Broker Address">
            <input type='number' name='mqtt_port' value='%MQTT_PORT%' placeholder="Port">
            <input type='text' name='mqtt_user' value='%MQTT_USER%' placeholder="User (Optional)">
            <input type='password' name='mqtt_pass' value='%MQTT_PASS%' placeholder="Password (Optional)">
        </div>

        <a href="/mqtt_config" class="btn-group" style="display:block; text-align:center; background:#2563eb; color:white; padding:10px; border-radius:6px; margin:15px 0;">Configure MQTT Topics</a>

        <h3>Day/Night Switch Time (Hour 0-23)</h3>
        <div class="grid-2-col">
            <input type='number' name='day_start_hour' value='%DAY_START%' min="0" max="23" placeholder="Day Start (Default 6)">
            <input type='number' name='day_end_hour' value='%DAY_END%' min="0" max="23" placeholder="Night Start (Default 18)">
        </div>

        <input type='submit' value='Save & Restart' style="margin-top: 20px;">
      </form>
    </div>

    <div class="card">
      <h2>System</h2>
      <form action='/reboot' method='POST' onsubmit='return confirm("Are you sure you want to reboot?");'>
        <input type='submit' value='Reboot Device' class="btn-danger">
      </form>
    </div>

    <div class="footer">
      <p>Build: %BUILD_DATE% %BUILD_TIME%</p>
      <p>
        <a href='/files'>File Manager</a> &bull; 
        <a href='/update'>Firmware Update</a>
      </p>
    </div>
  </div>
</body>
</html>
)rawliteral";

const char UPDATE_HTML_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>Firmware Update</title>
  %CSS%
</head>
<body>
  <div class="container">
    <div class="card">
      <h2>Firmware Update</h2>
      <p style="color:var(--text-light); margin-bottom:20px;">Select a .bin file to update the device firmware.</p>
      
      <form method='POST' action='/update' enctype='multipart/form-data'>
        <input type='file' name='update'>
        <input type='submit' value='Start Update' class="btn-danger">
      </form>
    </div>
    <div class="footer">
      <a href='/'>&laquo; Back to Home</a>
    </div>
  </div>
</body>
</html>
)rawliteral";

const char FILE_MANAGER_HTML_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>File Manager</title>
  %CSS%
</head>
<body>
  <div class="container">
    <div class="card">
      <h2>File Manager</h2>
      <form action='/upload' method='POST' enctype='multipart/form-data'>
        <h3>Upload Icon</h3>
        <input type='file' name='file' accept='.bmp'>
        <input type='submit' value='Upload BMP'>
      </form>
    </div>

    <div class="card">
      <h2>Files</h2>
      <ul>
        %FILE_LIST%
      </ul>
    </div>
    
    <div class="footer">
      <a href='/'>&laquo; Back to Home</a>
    </div>
  </div>
</body>
</html>
)rawliteral";

const char MQTT_CONFIG_HTML_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>MQTT Configuration</title>
  %CSS%
  <style>
    .topic-desc { font-size: 0.85em; color: var(--text-light); margin-top: -10px; margin-bottom: 15px; display: block; }
    code { background: #e5e7eb; padding: 2px 4px; border-radius: 4px; font-family: monospace; font-size: 0.9em; }
    pre { background: #f3f4f6; padding: 10px; border-radius: 6px; overflow-x: auto; font-size: 0.85em; border: 1px solid var(--border); }
  </style>
</head>
<body>
  <div class="container">
    <div class="card">
      <h2>MQTT Topics Configuration</h2>
      <form action='/saveConfig' method='POST'>
        
        <h3>Unified Topic (Recommended)</h3>
        <input type='text' name='mqtt_unified_topic' value='%MQTT_UNIFIED%' placeholder="epd/unified">
        <span class="topic-desc">
          Single topic for all data. JSON format:
          <pre>
{
  "date": { "solar": "...", "lunar": "..." },
  "weather": [ { "temp": "...", "icon": "..." } ],
  "env": { "temp": "...", "humi": "..." },
  "air": { "pm2p5": "...", "category": "..." }
}</pre>
        </span>

        <h3>Weather Topic</h3>
        <input type='text' name='mqtt_weather_topic' value='%MQTT_WEATHER%' placeholder="epd/weather">
        <span class="topic-desc">JSON: <code>[  { "date": "2023-10-27", "temp": "22/15",  "textDay": "晴", "iconDay": "100", "textNight": "晴", "iconNight": "150", "windDir": "西北风", "windScale": "3" }, ...]</code></span>
        <h3>Date/Calendar Topic</h3>
        <input type='text' name='mqtt_date_topic' value='%MQTT_DATE%' placeholder="epd/date">
        <span class="topic-desc">JSON: <code>{"阳历日期":"...", "农历日期":"...", "星期":"...", "节气信息":"..."}</code></span>

        <h3>Environment Topic</h3>
        <input type='text' name='mqtt_env_topic' value='%MQTT_ENV%' placeholder="epd/env">
        <span class="topic-desc">JSON: <code>{"temp":"25.5", "humi":"60"}</code></span>

        <h3>Air Quality Topic</h3>
        <input type='text' name='mqtt_air_quality_topic' value='%MQTT_AQI%' placeholder="epd/air_quality">
        <span class="topic-desc">JSON: <code>{"pm2p5":"25", "category":"Good"}</code></span>

        <h3>Calendar Events Topic</h3>
        <input type='text' name='mqtt_calendar_topic' value='%MQTT_CALENDAR%' placeholder="epd/calendar">
        <span class="topic-desc">JSON: <code>{"events":[{"start":"...", "summary":"..."}]}</code></span>

        <h3>Shift Schedule Topic</h3>
        <input type='text' name='mqtt_shift_topic' value='%MQTT_SHIFT%' placeholder="epd/shift">
        <span class="topic-desc">JSON: <code>{"date":"...", "shift":"..."}</code> or Array</span>

        <h3>Text Message Topic</h3>
        <input type='text' name='mqtt_topic' value='%MQTT_TOPIC%' placeholder="epd/text">
        <span class="topic-desc">Simple text string to display directly on screen.</span>

        <h3>Periodic Request Topic</h3>
        <input type='text' name='mqtt_request_topic' value='%MQTT_REQUEST%' placeholder="epd/weatherrequest">
        <span class="topic-desc">The device publishes "get" to this topic every N minutes (configured in Request Interval). Subscribe to this topic in Node-RED to trigger data push.</span>

        <div class="btn-group" style="display: flex; gap: 10px;">
            <button type="button" onclick="window.location.href='/'" style="background:#6b7280; flex:1; padding:12px; font-size:16px;">Cancel</button>
            <button type="submit" style="background:#2563eb; flex:1; padding:12px; font-size:16px;">Save & Restart</button>
        </div>
      </form>
    </div>
    
    <div class="footer">
      <a href='/'>&laquo; Back to Home</a>
    </div>
  </div>
</body>
</html>
)rawliteral";

// Function Declarations
void handleRoot();
void handleMqttConfig();
void handleFileManager();
void handleUpload();
void handleDelete();
void handleReboot();
void handleSaveConfig();
void handleSetPage();
void handleSetText();
void handleUpdate();
void handleUpdateFirmware();

#endif
