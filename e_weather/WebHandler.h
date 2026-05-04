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
extern float getBatteryVoltage();
extern void displayMessageWithBitmap(String text, const unsigned char* bitmap, int bitmapWidth, int bitmapHeight);
extern void displayWeatherDashboard(bool partial_update, bool sendSignal = false);
extern bool webOtaInProgress;
extern bool webOtaUploadFinished;
extern bool webOtaUploadSucceeded;
extern bool webOtaHasError;
extern size_t webOtaBytesWritten;
extern unsigned long webOtaStartMillis;

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
  h1 { font-size: 1.5rem; margin-bottom: 1rem; color: var(--text); text-align: center; }
  h2 { font-size: 1.25rem; margin-top: 0; margin-bottom: 1rem; padding-bottom: 0.5rem; border-bottom: 1px solid var(--border); text-align: center; }
  h3 { font-size: 1rem; margin-top: 1rem; margin-bottom: 0.5rem; color: var(--text-light); text-transform: uppercase; letter-spacing: 0.05em; font-weight: 600; text-align: center; }
  
  input[type=text], input[type=number], input[type=password], input[type=file], select {
    width: 100%;
    padding: 10px;
    margin: 5px 0 15px;
    border: 1px solid var(--border);
    border-radius: 6px;
    box-sizing: border-box;
    font-size: 14px;
    transition: border-color 0.2s;
  }
  input[type=text]:focus, input[type=number]:focus, input[type=password]:focus, select:focus {
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
  .grid-3-col { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; }
  .grid-3-col input { margin: 0 !important; }
  @media (max-width: 480px) { 
    .grid-2-col { grid-template-columns: 1fr; } 
    .grid-3-col { grid-template-columns: 1fr; }
  }
  
  a { color: var(--primary); text-decoration: none; }
  a:hover { text-decoration: underline; }
  
  .footer { text-align: center; color: var(--text-light); font-size: 0.875rem; margin-top: 40px; }
  ul { list-style: none; padding: 0; }
  li { padding: 10px 0; border-bottom: 1px solid var(--border); display: flex; justify-content: space-between; align-items: center; }
  li:last-child { border-bottom: none; }
  .badge { background: #e5e7eb; padding: 2px 6px; border-radius: 4px; font-size: 12px; }
</style>
<script>
  async function submitConfig(e, form) {
    if (e) e.preventDefault(); // Correct way to prevent default form submission
    
    const formData = new FormData(form);
    const submitBtn = form.querySelector('input[type="submit"], button[type="submit"]');
    const originalBtnText = submitBtn ? (submitBtn.value || submitBtn.innerText) : 'Save';
    
    if (submitBtn) {
      submitBtn.disabled = true;
      if (submitBtn.tagName === 'INPUT') submitBtn.value = 'Saving...';
      else submitBtn.innerText = 'Saving...';
    }
    
    try {
      const response = await fetch(form.action, {
        method: 'POST',
        body: formData
      });
      
      if (response.ok) {
        alert('Configuration saved successfully!');
      } else {
        alert('Failed to save configuration. Status: ' + response.status);
      }
    } catch (error) {
      alert('Error: ' + error.message);
    } finally {
      if (submitBtn) {
        submitBtn.disabled = false;
        if (submitBtn.tagName === 'INPUT') submitBtn.value = originalBtnText;
        else submitBtn.innerText = originalBtnText;
      }
    }
    return false;
  }
</script>
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
      <div style="display: flex; justify-content: center; gap: 15px; color: var(--text-light); font-size: 0.9rem;">
        %BATTERY_INFO%
        <span>Build: <strong>%BUILD_DATE% %BUILD_TIME%</strong></span>
      </div>
    </div>

    <div class="card">
      <form action='/saveConfig' method='POST' onsubmit='return submitConfig(event, this);'>
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

        <h3>ADC Settings (Battery)</h3>
        <div class="grid-3-col">
            <div>
                <label>ADC Pin</label>
                <input type='number' name='adc_pin' value='%ADC_PIN%' placeholder="34">
            </div>
            <div>
                <label>Divider Ratio</label>
                <input type='number' name='adc_ratio' step="0.01" value='%ADC_RATIO%' placeholder="2.0">
            </div>
            <div>
                <label>Threshold (V)</label>
                <input type='number' name='low_battery_threshold' step="0.1" value='%LOW_BATTERY_THRESHOLD%' placeholder="3.3">
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
        
        <h3>Refresh Settings</h3>
        <div class="grid-2-col">
            <div>
                <label>Full Refresh (min)</label>
                <input type='number' name='full_refresh_period' value='%FULL_REFRESH%' placeholder="0 = Disabled">
            </div>
            <div>
                <label>Battery Publish (min)</label>
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
        <label style="display:flex;align-items:center;gap:8px;cursor:pointer;margin:12px 0 4px;">
            <input type="checkbox" name="mqtt_protocol" value="1" %MQTT_PROTOCOL_CHECKED% style="width:auto;margin:0;">
            Use MQTT over TLS / MQTTS
        </label>

        <a href="/mqtt_config" class="btn-group" style="display:block; text-align:center; background:#2563eb; color:white; padding:10px; border-radius:6px; margin:15px 0;">Configure MQTT Topics</a>

        <h3>Day/Night Switch Time (Hour 0-23)</h3>
        <div class="grid-2-col">
            <input type='number' name='day_start_hour' value='%DAY_START%' min="0" max="23" placeholder="Day Start (Default 6)">
            <input type='number' name='day_end_hour' value='%DAY_END%' min="0" max="23" placeholder="Night Start (Default 18)">
        </div>

        <input type='submit' value='Save Configuration' style="margin-top: 20px;">
      </form>
    </div>

    <div class="card">
      <h2>System</h2>
      <form action='/reboot' method='POST' onsubmit='return confirm("Are you sure you want to reboot?");'>
        <input type='submit' value='Reboot Device' class="btn-danger">
      </form>
    </div>

    <div class="footer">
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
</head>
<body>
  <div class="container">
    <div class="card">
      <h2>MQTT Topics Configuration</h2>
      <form action='/saveConfig' method='POST' onsubmit='return submitConfig(event, this);'>
        
        <h3>Unified Topic</h3>
        <input type='text' name='mqtt_unified_topic' value='%MQTT_UNIFIED%' placeholder="epd/unified">

        <h3>Battery Voltage Topic</h3>
        <input type='text' name='mqtt_battery_topic' value='%MQTT_BATTERY%' placeholder="epd/battery">

        <h3>Hourly Forecast Topic</h3>
        <input type='text' name='mqtt_hourly_topic' value='%MQTT_HOURLY%' placeholder="epd/hourly">

        <div class="btn-group" style="display: flex; gap: 10px;">
            <button type="button" onclick="window.location.href='/'" style="background:#6b7280; flex:1; padding:12px; font-size:16px;">Cancel</button>
            <button type="submit" style="background:#2563eb; flex:1; padding:12px; font-size:16px;">Save Config</button>
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
void handleUpdate();
void handleUpdateFirmware();

#endif
