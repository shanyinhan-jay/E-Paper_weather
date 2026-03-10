/*

- SCK : 13
- MOSI : 14
- CS : 15
- RST : 26
- DC : 27
- BUSY : 25
- ADC :34
- BATTERY MODE : 4
*/
// Serial2 Communication


#include "DEV_Config.h"
#include "Local_EPD_4in2.h"
#include "GUI_Paint.h"

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include <stdlib.h>
// #include <LittleFS.h> // Already included
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "icons.h"
#include "AppConfig.h"
#include "WebHandler.h"

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <OneButton.h>
#include <vector>

extern const uint8_t u8g2_font_wqy12_t_gb2312[] U8X8_PROGMEM;
// extern const uint8_t u8g2_font_wqy16_t_gb2312[] U8X8_PROGMEM;
// extern const uint8_t u8g2_font_wqy14_t_gb2312[] U8X8_PROGMEM; 
extern const uint8_t u8g2_font_open_iconic_weather_6x_t[] U8X8_PROGMEM;
extern const uint8_t u8g2_font_open_iconic_weather_4x_t[] U8X8_PROGMEM;
extern const uint8_t u8g2_font_logisoso60_tf[] U8X8_PROGMEM;
extern const uint8_t u8g2_font_logisoso30_tf[] U8X8_PROGMEM;



const char* build_date = __DATE__;
const char* build_time = __TIME__;

WebServer server(80);

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP); 
UBYTE *BlackImage = NULL;
OneButton button(BUTTON_PIN, true); // GPIO defined in DEV_Config.h, Active Low

const char* DEFAULT_AP_SSID_BASE = "EPD-Display";
String ap_ssid = DEFAULT_AP_SSID_BASE;

#define MAX_CALENDAR_EVENTS 20
#define MAX_SHIFT_EVENTS 100

Config config;

struct CalendarEvent {
    time_t start_time;
    String summary;
};

struct ShiftEvent {
    int year;
    int month;
    int day;
    String content;
};

struct WeatherData {
    String date;
    String temp;
    String cond_day;
    String cond_night;
    String wind_dir;
    String wind_sc;
    String icon_day;
    String icon_night;
};

// Global Weather Data
std::vector<CalendarEvent> calendarEvents;
std::vector<ShiftEvent> shiftEvents;
WeatherData currentForecast[7];
int currentForecastCount = 0;
String currentCity = "绍兴";
String solarDate = "";
String weekDay = "";
String lunarDate = "";
String termInfo = "";
String indoorTemp = "";
String indoorHumi = "";
String airPm2p5 = "";
String airCategory = "";
int lastMinute = -1;

// Flags for deferred display update
bool updateWeatherPending = false;
bool updateCalendarPending = false;
bool updateEnvPending = false;
bool updateDatePending = false;
bool fullRefreshPending = false; // Flag for full refresh instead of partial
unsigned long lastUpdateTrigger = 0;
const unsigned long UPDATE_DELAY_MS = 200; // Wait 200ms after last message to update (debounce)

Page currentPage = PAGE_WEATHER;
bool switchPagePending = false;

// Battery Mode & Deep Sleep Globals
bool refreshDone = false;
unsigned long lastRefreshTime = 0; // Timestamp of last display update
bool configMode = false;
unsigned long wakeTime = 0;

// Forward declaration
void enterDeepSleep();

class Paint_GFX : public Adafruit_GFX {
public:
  float scale = 1.0;
  Paint_GFX() : Adafruit_GFX(EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT) {}
  
  void setScale(float s) {
      scale = (s > 0) ? s : 1.0;
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    uint16_t targetColor;
    if (config.invert_display) {
        // Inverted: Foreground(1) -> WHITE, Background(0) -> BLACK
        targetColor = (color == 1) ? WHITE : BLACK;
    } else {
        // Normal: Foreground(1) -> BLACK, Background(0) -> WHITE
        targetColor = (color == 1) ? BLACK : WHITE;
    }

    if (scale == 1.0) {
        if (x < 0 || x >= EPD_4IN2_WIDTH || y < 0 || y >= EPD_4IN2_HEIGHT) return;
        Paint_SetPixel(x, y, targetColor);
    } else {
        // Scaled Drawing with Float
        int16_t start_x = floor(x * scale);
        int16_t end_x = ceil((x + 1) * scale);
        int16_t start_y = floor(y * scale);
        int16_t end_y = ceil((y + 1) * scale);
        
        for (int16_t sy = start_y; sy < end_y; sy++) {
            for (int16_t sx = start_x; sx < end_x; sx++) {
                if (sx < 0 || sx >= EPD_4IN2_WIDTH || sy < 0 || sy >= EPD_4IN2_HEIGHT) continue;
                Paint_SetPixel(sx, sy, targetColor);
            }
        }
    }
  }

  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
      int16_t temp = x0; x0 = y0; y0 = temp;
      temp = x1; x1 = y1; y1 = temp;
    }

    if (x0 > x1) {
      int16_t temp = x0; x0 = x1; x1 = temp;
      temp = y0; y0 = y1; y1 = temp;
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
      ystep = 1;
    } else {
      ystep = -1;
    }

    for (; x0<=x1; x0++) {
      if (steep) {
        drawPixel(y0, x0, color);
      } else {
        drawPixel(x0, y0, color);
      }
      err -= dy;
      if (err < 0) {
        y0 += ystep;
        err += dx;
      }
    }
  }

  void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
      for(int16_t y = -r; y <= r; y++) {
          for(int16_t x = -r; x <= r; x++) {
              if(x*x + y*y <= r*r) {
                  drawPixel(x0+x, y0+y, color);
              }
          }
      }
  }
};

Paint_GFX paint_gfx;
U8G2_FOR_ADAFRUIT_GFX u8g2;

void loadConfig() {
  if (LittleFS.begin(true)) {
    if (LittleFS.exists("/config.json")) {
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        configFile.close();

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, buf.get());
        if (!error) {
          strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "", sizeof(config.wifi_ssid));
          strlcpy(config.wifi_pass, doc["wifi_pass"] | "", sizeof(config.wifi_pass));
          strlcpy(config.mqtt_server, doc["mqtt_server"] | "", sizeof(config.mqtt_server));
          config.mqtt_port = doc["mqtt_port"] | 1883;
          strlcpy(config.mqtt_user, doc["mqtt_user"] | "", sizeof(config.mqtt_user));
          strlcpy(config.mqtt_pass, doc["mqtt_pass"] | "", sizeof(config.mqtt_pass));
          strlcpy(config.mqtt_topic, doc["mqtt_topic"] | "epd/text", sizeof(config.mqtt_topic));
          strlcpy(config.mqtt_weather_topic, doc["mqtt_weather_topic"] | "epd/weather", sizeof(config.mqtt_weather_topic));
          strlcpy(config.mqtt_date_topic, doc["mqtt_date_topic"] | "epd/date", sizeof(config.mqtt_date_topic));
          strlcpy(config.mqtt_env_topic, doc["mqtt_env_topic"] | "epd/env", sizeof(config.mqtt_env_topic));
          strlcpy(config.mqtt_calendar_topic, doc["mqtt_calendar_topic"] | "epd/calendar", sizeof(config.mqtt_calendar_topic));
          strlcpy(config.mqtt_shift_topic, doc["mqtt_shift_topic"] | "epd/shift", sizeof(config.mqtt_shift_topic));
          strlcpy(config.mqtt_air_quality_topic, doc["mqtt_air_quality_topic"] | "epd/air_quality", sizeof(config.mqtt_air_quality_topic));
          strlcpy(config.mqtt_unified_topic, doc["mqtt_unified_topic"] | "epd/unified", sizeof(config.mqtt_unified_topic));
          strlcpy(config.mqtt_request_topic, doc["mqtt_request_topic"] | "epd/weatherrequest", sizeof(config.mqtt_request_topic));
          strlcpy(config.ntp_server, doc["ntp_server"] | "ntp1.aliyun.com", sizeof(config.ntp_server));
          strlcpy(config.ntp_server_2, doc["ntp_server_2"] | "ntp2.aliyun.com", sizeof(config.ntp_server_2));
          config.full_refresh_period = doc["full_refresh_period"] | 0;
          config.request_interval = doc["request_interval"] | 30;
          config.day_start_hour = doc["day_start_hour"] | 6;
          config.day_end_hour = doc["day_end_hour"] | 18;
          config.invert_display = doc["invert_display"] | false;
          config.ui_mode = doc["ui_mode"] | 0;
          config.adc_pin = doc["adc_pin"] | 34;
          config.adc_ratio = doc["adc_ratio"] | 2.0;
          config.low_battery_threshold = doc["low_battery_threshold"] | 3.3;
          config.sleep_delay = doc["sleep_delay"] | 10;
          config.config_timeout = doc["config_timeout"] | 5;
          
          config.use_static_ip = doc["use_static_ip"] | false;
          strlcpy(config.static_ip, doc["static_ip"] | "", sizeof(config.static_ip));
          strlcpy(config.static_gw, doc["static_gw"] | "", sizeof(config.static_gw));
          strlcpy(config.static_mask, doc["static_mask"] | "255.255.255.0", sizeof(config.static_mask));
          strlcpy(config.static_dns, doc["static_dns"] | "114.114.114.114", sizeof(config.static_dns));
        }
      }
    }
  }

  // Pre-allocate BlackImage to avoid fragmentation
  if (BlackImage == NULL) {
      UWORD Imagesize = ((EPD_4IN2_WIDTH % 8 == 0) ? (EPD_4IN2_WIDTH / 8 ) : (EPD_4IN2_WIDTH / 8 + 1)) * EPD_4IN2_HEIGHT;
      BlackImage = (UBYTE *)malloc(Imagesize);
      if (BlackImage == NULL) {
          Serial.println("FATAL: BlackImage Malloc Failed in Setup!");
      } else {
          Serial.printf("BlackImage Allocated: %u bytes\n", Imagesize);
          // Initialize with White
          memset(BlackImage, 0xFF, Imagesize);
      }
  }
}

void saveConfig() {
  JsonDocument doc;
  doc["wifi_ssid"] = config.wifi_ssid;
  doc["wifi_pass"] = config.wifi_pass;
  doc["mqtt_server"] = config.mqtt_server;
  doc["mqtt_port"] = config.mqtt_port;
  doc["mqtt_user"] = config.mqtt_user;
  doc["mqtt_pass"] = config.mqtt_pass;
  doc["mqtt_topic"] = config.mqtt_topic;
  doc["mqtt_weather_topic"] = config.mqtt_weather_topic;
  doc["mqtt_date_topic"] = config.mqtt_date_topic;
  doc["mqtt_env_topic"] = config.mqtt_env_topic;
  doc["mqtt_calendar_topic"] = config.mqtt_calendar_topic;
  doc["mqtt_shift_topic"] = config.mqtt_shift_topic;
  doc["mqtt_air_quality_topic"] = config.mqtt_air_quality_topic;
  doc["mqtt_unified_topic"] = config.mqtt_unified_topic;
  doc["mqtt_request_topic"] = config.mqtt_request_topic;
  doc["ntp_server"] = config.ntp_server;
  doc["ntp_server_2"] = config.ntp_server_2;
  doc["full_refresh_period"] = config.full_refresh_period;
  doc["request_interval"] = config.request_interval;
  doc["day_start_hour"] = config.day_start_hour;
  doc["day_end_hour"] = config.day_end_hour;
  doc["invert_display"] = config.invert_display;
  doc["ui_mode"] = config.ui_mode;
  doc["adc_pin"] = config.adc_pin;
  doc["adc_ratio"] = config.adc_ratio;
  doc["low_battery_threshold"] = config.low_battery_threshold;
  doc["sleep_delay"] = config.sleep_delay;
  doc["config_timeout"] = config.config_timeout;
  
  doc["use_static_ip"] = config.use_static_ip;
  doc["static_ip"] = config.static_ip;
  doc["static_gw"] = config.static_gw;
  doc["static_mask"] = config.static_mask;
  doc["static_dns"] = config.static_dns;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    return;
  }
  serializeJson(doc, configFile);
  configFile.close();
}

float getBatteryVoltage() {
    if (config.adc_pin < 0) return 0.0;
    
    Serial.println("getBatteryVoltage called");
    // Smooth reading with multiple samples
    uint32_t sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(config.adc_pin);
        delay(10);
    }
    float avgAdc = sum / 10.0;
    
    // 12-bit ADC (0-4095) with 3.3V reference
    // Voltage = ADC * (3.3 / 4095.0) * ratio
    float voltage = avgAdc * (3.3 / 4095.0) * config.adc_ratio;
    return voltage;
}

void displayMessage(String text) {
    Serial.println("displayMessage start");
    Serial.printf("Free Heap: %u\n", ESP.getFreeHeap());
    
    DEV_Module_Init();
    Local_EPD_4IN2_Init();
    
    if (BlackImage == NULL) {
         Serial.println("Error: BlackImage is NULL (Should be allocated in setup)");
         // Try emergency alloc
         UWORD Imagesize = ((EPD_4IN2_WIDTH % 8 == 0) ? (EPD_4IN2_WIDTH / 8 ) : (EPD_4IN2_WIDTH / 8 + 1)) * EPD_4IN2_HEIGHT;
         BlackImage = (UBYTE *)malloc(Imagesize);
         if (BlackImage == NULL) Serial.println("BlackImage Malloc Failed");
         else Serial.println("BlackImage Malloc Success (Emergency)");
    }
    
    if (BlackImage != NULL) {
        Serial.println("Preparing Image");
        UWORD InitColor = config.invert_display ? BLACK : WHITE;
        Paint_NewImage(BlackImage, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, 0, InitColor);
        Paint_SelectImage(BlackImage);
        Paint_Clear(InitColor);
        
        Serial.println("U8G2 begin");
        u8g2.begin(paint_gfx);
        
        // --- Enable 1.0x Scaling ---
        float scale = 1.0;
        paint_gfx.setScale(scale); 
        int logicalWidth = EPD_4IN2_WIDTH / scale;
        int logicalHeight = EPD_4IN2_HEIGHT / scale;
        
        // u8g2.setFont(u8g2_font_wqy16_t_gb2312); // Use WenQuanYi 16pt GB2312 font
        // u8g2.setFont(u8g2_font_wqy14_t_gb2312); // Use WenQuanYi 14pt GB2312 font
        u8g2.setFont(u8g2_font_wqy12_t_gb2312); // Use WenQuanYi 12pt GB2312 font
        u8g2.setFontMode(1); // Transparent mode
        u8g2.setForegroundColor(1); // Black
        u8g2.setBackgroundColor(0); // White
        
        Serial.println("Processing Text");
        // --- 1. Wrap Text into Lines ---
        std::vector<String> lines;
        String currentLine = "";
        int len = text.length();
        int i = 0;
        
        while (i < len) {
            
            // Handle explicit newline
            if (text[i] == '\n') {
                lines.push_back(currentLine);
                currentLine = "";
                i++;
                continue;
            }
            
            int charLen = 1;
            unsigned char c = text[i];
            if (c >= 0xF0) charLen = 4;
            else if (c >= 0xE0) charLen = 3;
            else if (c >= 0xC0) charLen = 2;
            
            if (i + charLen > len) {
                // Incomplete multibyte char at end
                break; 
            }

            String nextChar = text.substring(i, i + charLen);
            
            // Basic bounds check to prevent crash in getUTF8Width
            if (nextChar.length() > 0) {
                 if (u8g2.getUTF8Width((currentLine + nextChar).c_str()) > (logicalWidth - 10)) {
                    lines.push_back(currentLine);
                    currentLine = nextChar;
                 } else {
                    currentLine += nextChar;
                 }
            }
            i += charLen;
        }
        if (currentLine.length() > 0) {
            lines.push_back(currentLine);
        }
        
        Serial.println("Calculating Layout");
        // --- 2. Calculate Centering ---
        int lineHeight = 20; // Fixed line height for safety
        int totalHeight = lines.size() * lineHeight;
        
        int startY = (logicalHeight - totalHeight) / 2 + 16;
        
        if (startY < 20) startY = 20; // Safety clamp

        // --- 3. Draw Lines (Centered) ---
        Serial.println("Drawing Lines");
        for (const String& line : lines) {
            int w = u8g2.getUTF8Width(line.c_str());
            int x = (logicalWidth - w) / 2;
            if (x < 0) x = 0;
            
            u8g2.drawUTF8(x, startY, line.c_str());
            
            startY += lineHeight;
        }
        
        // Reset Scale
        paint_gfx.setScale(1.0);

        Serial.println("Displaying");
        Local_EPD_4IN2_Display(BlackImage);
        hibernateEPD();
        // free(BlackImage); // Keep allocated to prevent fragmentation
        // BlackImage = NULL;
        Serial.println("displayMessage done");
    }
}

void displayCalendarPage(bool partial_update = false) {
    Serial.println("Displaying Calendar Page...");
    Serial.print("Shift Events count: ");
    Serial.println(shiftEvents.size());

    DEV_Module_Init();
    
    if (BlackImage == NULL) {
         Serial.println("Error: BlackImage is NULL (Should be allocated in setup)");
         // Try emergency alloc
         UWORD Imagesize = ((EPD_4IN2_WIDTH % 8 == 0) ? (EPD_4IN2_WIDTH / 8 ) : (EPD_4IN2_WIDTH / 8 + 1)) * EPD_4IN2_HEIGHT;
         BlackImage = (UBYTE *)malloc(Imagesize);
         if (BlackImage == NULL) Serial.println("BlackImage Malloc Failed (Calendar)");
    }
    
    if (BlackImage != NULL) {
        UWORD InitColor = config.invert_display ? BLACK : WHITE;
        Paint_NewImage(BlackImage, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, 0, InitColor);
        Paint_SelectImage(BlackImage);
        Paint_Clear(InitColor);
        
        u8g2.begin(paint_gfx);
        u8g2.setFontMode(1); 
        u8g2.setForegroundColor(1);
        u8g2.setBackgroundColor(0);
        
        // Title removed as per request
        // u8g2.setFont(u8g2_font_wqy16_t_gb2312);
        // u8g2.drawUTF8(10, 20, "两周日历 (Two Weeks)");
        // paint_gfx.drawFastHLine(0, 25, EPD_4IN2_WIDTH, 2);

        // Grid Settings
        int cols = 7;
        int rows = 2;
        int cellW = EPD_4IN2_WIDTH / cols;
        int cellH = EPD_4IN2_HEIGHT / rows; // Use full height since title is gone
        int startY = 0; // Start from top

        time_t now = timeClient.getEpochTime();
        const char* weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

        for (int i = 0; i < 14; i++) {
            time_t future = now + i * 86400;
            struct tm * t = gmtime(&future);
            
            int c = i % cols;
            int r = i / cols;
            
            int x = c * cellW;
             int y = startY + r * cellH;
             
             // Save "t" (current cell date) to local vars immediately because gmtime returns static pointer
             // which will be overwritten by other gmtime calls (e.g. inside event loop)
             int t_year = t->tm_year;
             int t_mon = t->tm_mon;
             int t_mday = t->tm_mday;
             int t_wday = t->tm_wday;
             
             // Draw Cell Border
            // paint_gfx.drawRect(x, y, cellW, cellH, 1);
            paint_gfx.drawFastHLine(x, y, cellW, 1);
            paint_gfx.drawFastHLine(x, y + cellH - 1, cellW, 1);
            paint_gfx.drawFastVLine(x, y, cellH, 1);
            paint_gfx.drawFastVLine(x + cellW - 1, y, cellH, 1);
            
            // Highlight Weekend
            // If it's Saturday (6) or Sunday (0), use black background for header
            if (t_wday == 0 || t_wday == 6) {
                 for(int k=0; k<20; k++) {
                     paint_gfx.drawFastHLine(x, y + k, cellW, 1);
                 }
                 u8g2.setForegroundColor(0); // White text
                 u8g2.setBackgroundColor(1);
            } else {
                 u8g2.setForegroundColor(1); // Black text
                 u8g2.setBackgroundColor(0);
            }

            // Draw Weekday
            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            String wdayStr = weekDays[t_wday];
            int wW = u8g2.getUTF8Width(wdayStr.c_str());
            u8g2.drawUTF8(x + (cellW - wW)/2, y + 14, wdayStr.c_str());
            
            // Draw horizontal line below weekday (header separator)
            paint_gfx.drawFastHLine(x, y + 20, cellW, 1);

            // Reset Color for Date
            u8g2.setForegroundColor(1);
            u8g2.setBackgroundColor(0);
            
            // Draw Date
            u8g2.setFont(u8g2_font_logisoso30_tf);
            String dateStr = String(t_mday);
            int dW = u8g2.getUTF8Width(dateStr.c_str());
           // Center date vertically in the remaining space below header
            // Header height is 20. Cell Height is ~85. Remaining is 65.
            // Move date up to avoid overlap with events
            u8g2.drawUTF8(x + (cellW - dW)/2, y + 60, dateStr.c_str());

            // Draw Event Marker or Summary (if any)
            // Check for Shift
            bool hasShift = false;
            String shiftText = "";
            for (const auto& sev : shiftEvents) {
                 if (sev.year == (t_year + 1900) && sev.month == (t_mon + 1) && sev.day == t_mday) {
                     hasShift = true;
                     shiftText = sev.content;
                     break;
                 }
            }
            
            // Check for Event
            bool hasEvent = false;
            CalendarEvent targetEvent;
            for (const auto& ev : calendarEvents) {
                 struct tm* evTm = gmtime(&ev.start_time);
                 if (evTm->tm_year == t_year && evTm->tm_mon == t_mon && evTm->tm_mday == t_mday) {
                     hasEvent = true;
                     targetEvent = ev;
                     break;
                 }
            }

            if (hasShift || hasEvent) {
                 // Draw separator line between date and content
                 int sepMargin = 5;
                 paint_gfx.drawFastHLine(x + sepMargin, y + 70, cellW - (2 * sepMargin), 1);
            }

            if (hasShift) {
                 u8g2.setFont(u8g2_font_wqy12_t_gb2312);
                 int sW = u8g2.getUTF8Width(shiftText.c_str());
                 // Draw centered below separator (y+70)
                 // Text baseline at y+86 (approx 16px height)
                 u8g2.drawUTF8(x + (cellW - sW)/2, y + 86, shiftText.c_str());
            }

            if (hasEvent) {
                 // Found event for this day
                 // Draw summary with word wrap (4 chars per line, max 5 lines)
                  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
                  String sum = targetEvent.summary;
                  
                  int lineHeight = 14;
                  int maxLines = 5;
                  int charsPerLine = 4;
                  int currentLine = 0;
                  // Start below Shift area. Shift ends at ~88. 
                  // If Shift exists, start at 105. If not, could start earlier but consistent alignment is better.
                  // User said "Event starts from line below Shift".
                  // Let's use y+100 as base.
                  int startY_Text = y + 100;
                  
                  int k = 0;
                  while (k < sum.length() && currentLine < maxLines) {
                       String lineStr = "";
                       int charsInThisLine = 0;
                       
                       // Collect chars for this line
                       while (k < sum.length() && charsInThisLine < charsPerLine) {
                           int charLen = 1;
                           unsigned char c = sum[k];
                           if (c >= 0xC0) charLen = 2;
                           if (c >= 0xE0) charLen = 3;
                           if (c >= 0xF0) charLen = 4;
                           
                           lineStr += sum.substring(k, k + charLen);
                           k += charLen;
                           charsInThisLine++;
                       }
                       
                       // Check if we need to truncate (if this is the last allowed line and there is more text)
                       if (currentLine == maxLines - 1 && k < sum.length()) {
                           // Force draw ".." on next line to indicate more content
                           u8g2.drawUTF8(x + (cellW - u8g2.getUTF8Width(".."))/2, startY_Text + (currentLine + 1) * lineHeight, "..");
                       }
                       
                       // Draw this line centered
                       int sW = u8g2.getUTF8Width(lineStr.c_str());
                       u8g2.drawUTF8(x + (cellW - sW)/2, startY_Text + currentLine * lineHeight, lineStr.c_str());
                       currentLine++;
                   }
            }
         }

        if (partial_update) {
            Local_EPD_4IN2_Init_Partial();
            Local_EPD_4IN2_PartialDisplay(0, 0, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, BlackImage);
        } else {
            Local_EPD_4IN2_Init();
            Local_EPD_4IN2_Display(BlackImage);
        }
        hibernateEPD();
        
        // free(BlackImage); // Keep allocated
        // BlackImage = NULL;
    }
}

char getIconChar(String cond) {
    char iconChar = 64; // Default Sun
    String c = cond;
    c.toLowerCase();
    
    // Chinese matches
    if (c.indexOf("多云") >= 0 || c.indexOf("阴") >= 0) iconChar = 65; // Cloud/Overcast
    else if (c.indexOf("雨") >= 0) iconChar = 67; // Rain
    else if (c.indexOf("雪") >= 0) iconChar = 67; // Snow
    else if (c.indexOf("雷") >= 0) iconChar = 69; // Thunder
    else if (c.indexOf("晴") >= 0) iconChar = 64; // Sun
    
    // English matches
    else if (c.indexOf("cloud") >= 0 || c.indexOf("overcast") >= 0) iconChar = 65; 
    else if (c.indexOf("rain") >= 0 || c.indexOf("drizzle") >= 0) iconChar = 67;
    else if (c.indexOf("storm") >= 0 || c.indexOf("thunder") >= 0) iconChar = 69;
    else if (c.indexOf("snow") >= 0) iconChar = 67;
    
    return iconChar;
}

void drawBmp(String filename, int x, int y, int scale = 1) {
    File bmpFile = LittleFS.open(filename, "r");
    if (!bmpFile) {
        Serial.print("File not found: ");
        Serial.println(filename);
        return;
    }

    if (bmpFile.read() != 'B' || bmpFile.read() != 'M') {
        Serial.println("Not a BMP file");
        bmpFile.close();
        return;
    }

    // Read header
    uint32_t dataOffset;
    bmpFile.seek(0x0A);
    bmpFile.read((uint8_t*)&dataOffset, 4);
    
    uint32_t width, height;
    bmpFile.seek(0x12);
    bmpFile.read((uint8_t*)&width, 4);
    bmpFile.read((uint8_t*)&height, 4);
    
    uint16_t depth;
    bmpFile.seek(0x1C);
    bmpFile.read((uint8_t*)&depth, 2);

    if (depth != 24 && depth != 32) {
        bmpFile.close();
        return;
    }
    
    // Auto-adjust scale if image is large (e.g. user provided high-res 2x image)
    // Original icon ~40px wide. If width > 60:
    // - If scale=2 (Main Icon), set scale=1 (display 1:1, full size 80x74)
    // - If scale=1 (Forecast), set downscale=true (display 1:2, half size 40x37)
    //
    // BUT user explicitly calls drawBmp(..., 1) for -L icons (which are large).
    // If width > 60 (e.g. 72) and scale is passed as 1.
    // The current logic: if (width > 60) { if (scale == 1) downscale = true; }
    // This forces scale 1 -> 0.5 (36x36) ! This is why user sees half size.
    
    // We need to differentiate between:
    // 1. "I want to draw a large icon normally" (scale 1 passed, large icon) -> Keep scale 1
    // 2. "I want to draw a large icon in a small spot" (scale 1 passed? no, usually we wouldn't)
    //
    // The previous logic assumed if we passed scale 1, we meant "small icon slot", so if image was large, we downscaled.
    // But now for Main Icon -L, we pass scale 1 explicitly.
    // And for Forecast Icon, we pass scale 1? No, forecast loop uses standard icons.
    // If forecast loop encountered a large icon (e.g. user replaced standard icon with large one), then scale 1 -> downscale is correct.
    
    // How to fix?
    // We can change the calling code to pass scale 0 for "auto"? No.
    // Or we remove this auto-downscale logic for scale 1?
    // If we remove it:
    // - Main Icon -L (72x72, scale 1) -> Draws 72x72. Correct.
    // - Forecast Icon (Standard 36x36, scale 1) -> Draws 36x36. Correct.
    // - Forecast Icon (User replaced with 72x72, scale 1) -> Draws 72x72. Too big for forecast slot!
    
    // So the auto-downscale is valuable for "User replaced small icon with large file".
    // But it breaks "User wants to draw large icon in large slot using scale 1".
    
    // Solution:
    // In displayWeatherDashboard, for -L icons, we can pass a special scale or just use scale 1.
    // If we want to keep auto-downscale for forecast, we need a way to say "Don't downscale".
    // Maybe pass scale = 0 to mean "Native size"?
    // Or check if y position implies Main Icon?
    // Or just change the logic:
    // If scale == 1 and width > 60 -> Downscale.
    // This is exactly what hurts us.
    
    // User calls: drawBmp(iconPathL, iconX, 30, 1);
    // Width = 72. Scale = 1. -> Downscale -> 36x36.
    
    // If we change call to: drawBmp(iconPathL, iconX, 30, 0); // 0 means native?
    // Or we just remove the downscale logic for scale 1?
    // If user puts 72x72 icon in forecast (where 36x36 is expected), it will overlap.
    // Maybe that's user error?
    // But the auto-logic was "smart".
    
    // Let's modify the logic to only downscale if we really need to?
    // Or better: In displayWeatherDashboard, call with scale = 2 for -L icon?
    // No, 72x72 * 2 = 144.
    
    // Let's change the condition.
    // If we pass scale 1, we usually mean "draw as is".
    // The auto-downscale for scale 1 was probably added to handle "Large icon in small slot".
    // But now we have a valid use case for "Large icon in large slot" (Main Dashboard).
    // Main Dashboard usually uses scale 2 for small icons.
    // So if we have a large icon, we want scale 1.
    
    // What if we use a flag? Or negative scale?
    // scale = -1 means "Force 1:1, no auto-downscale"?
    
    // Let's change the logic to:
    // If scale == 1 && width > 60:
    //    We assume it's for forecast slot?
    //    Forecast slot Y is usually > 100.
    //    Main slot Y is 30.
    //    We could check Y?
    //    if (y < 100) -> Main slot -> Don't downscale.
    //    if (y > 100) -> Forecast slot -> Downscale.
    
    bool downscale = false;
    if (width > 60) {
        if (scale == 2) {
            scale = 1;
        } else if (scale == 1) {
             // Only downscale if we are NOT in the main icon area (y=30)
             // This is a bit hacky but safe for this specific layout
             if (y > 60) { 
                 downscale = true;
             }
        }
    }

    bmpFile.seek(dataOffset);

    // BMP rows are padded to 4 bytes
    uint32_t rowSize = (width * (depth / 8) + 3) & ~3;
    uint8_t *rowBuff = new (std::nothrow) uint8_t[rowSize];
    if (!rowBuff) {
        Serial.println("Memory allocation failed for rowBuff");
        bmpFile.close();
        return;
    }

    // BMP is stored bottom-up
    for (int row = 0; row < height; row++) {
        bmpFile.read(rowBuff, rowSize);
        
        if (downscale && (row % 2 != 0)) continue; // Skip odd rows for downscaling
        
        // Target Y (flip vertically)
        int ty;
        if (downscale) {
             ty = y + (height - 1 - row) / 2;
        } else {
             ty = y + (height - 1 - row) * scale;
        }
        
        for (int col = 0; col < width; col++) {
             if (downscale && (col % 2 != 0)) continue; // Skip odd cols for downscaling

             int b, g, r;
             int pxOffset = col * (depth / 8);
             
             b = rowBuff[pxOffset];
             g = rowBuff[pxOffset+1];
             r = rowBuff[pxOffset+2];
             
             // Simple thresholding
             int brightness = (r + g + b) / 3;
             uint16_t color = (brightness < 128) ? 1 : 0; // 1 = Black, 0 = White
             
             // Draw scaled pixel
             if (downscale) {
                 paint_gfx.drawPixel(x + col / 2, ty, color);
             } else {
                 for (int dy = 0; dy < scale; dy++) {
                     for (int dx = 0; dx < scale; dx++) {
                         paint_gfx.drawPixel(x + col * scale + dx, ty + dy, color);
                     }
                 }
             }
        }
    }
    
    delete[] rowBuff;
    bmpFile.close();
}

// Add font declaration
extern const uint8_t u8g2_font_logisoso50_tf[] U8G2_FONT_SECTION("u8g2_font_logisoso50_tf");

void drawIconFromProgmem(const unsigned char* data, int x, int y, int w, int h, int scale = 1) {
    if (!data) return;

    // Auto-adjust scale if image is large (e.g. user provided high-res 2x image)
    bool downscale = false;
    if (w > 60) {
        if (scale == 2) {
            scale = 1;      // Main Icon: Use full resolution
        } else if (scale == 1) {
             // Only downscale if we are NOT in the main icon area (y=30)
             if (y > 60) { 
                 downscale = true; // Forecast Icon: Downscale to 50%
             }
        }
    }
    
    // Calculate bytes per row (stride) based on width
    // Standard bitmap format: rows are byte-aligned
    int bytesPerRow = (w + 7) / 8;
    
    for (int row = 0; row < h; row++) {
        if (downscale && (row % 2 != 0)) continue; // Skip odd rows

        for (int col = 0; col < w; col++) {
            if (downscale && (col % 2 != 0)) continue; // Skip odd cols

            // Calculate byte index and bit index
            int byteIdx = row * bytesPerRow + (col / 8);
            int bitIdx = 7 - (col % 8); // MSB first
            
            uint8_t b = pgm_read_byte(&data[byteIdx]);
            
            if (b & (1 << bitIdx)) {
                // Draw pixel with scaling
                if (downscale) {
                    paint_gfx.drawPixel(x + col / 2, y + row / 2, 1);
                } else {
                    if (scale == 1) {
                        paint_gfx.drawPixel(x + col, y + row, 1);
                    } else {
                        // Draw a block for scaled pixel
                        for (int dy = 0; dy < scale; dy++) {
                            for (int dx = 0; dx < scale; dx++) {
                                paint_gfx.drawPixel(x + col * scale + dx, y + row * scale + dy, 1);
                            }
                        }
                    }
                }
            }
        }
    }
}

void displayWeatherDashboard(bool partial_update, bool sendSignal) {
    DEV_Module_Init();
    
    if (BlackImage == NULL) {
         Serial.println("Error: BlackImage is NULL (Should be allocated in setup)");
         // Try emergency alloc
         UWORD Imagesize = ((EPD_4IN2_WIDTH % 8 == 0) ? (EPD_4IN2_WIDTH / 8 ) : (EPD_4IN2_WIDTH / 8 + 1)) * EPD_4IN2_HEIGHT;
         BlackImage = (UBYTE *)malloc(Imagesize);
         if (BlackImage == NULL) Serial.println("BlackImage Malloc Failed (Weather)");
    }
    
    if (BlackImage != NULL) {
        UWORD InitColor = config.invert_display ? BLACK : WHITE;
        Paint_NewImage(BlackImage, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, 0, InitColor);
        Paint_SelectImage(BlackImage);
        Paint_Clear(InitColor);
        
        u8g2.begin(paint_gfx);
        u8g2.setFontMode(1); 
        u8g2.setForegroundColor(1);
        u8g2.setBackgroundColor(0);

        // --- Layout ---
        // 顶部垂直分割线 (x=200, y=5-145)
        paint_gfx.drawFastVLine(200, 5, 135, 1); 

        // Draw Thermometer Icon (Top Right)
        drawIconFromProgmem(gImage_tem, 375, 5, 20, 36, 1);
        
        // === LEFT SIDE (Date & Time) ===
        if (config.ui_mode == 0) {
            // --- TIME MODE ---
            u8g2.setFont(u8g2_font_logisoso50_tf); 
            String timeStr = timeClient.getFormattedTime().substring(0, 5); 
            int tWidth = u8g2.getUTF8Width(timeStr.c_str());
            int timeX = 100 - (tWidth / 2);
            u8g2.drawUTF8(timeX, 65, timeStr.c_str());
            
            // Check for today's event and draw bell icon
            // Only trigger for Calendar Events (Schedule), NOT Shifts
            time_t now = timeClient.getEpochTime();
            struct tm *t_now = gmtime(&now);
            int today_year = t_now->tm_year;
            int today_mon = t_now->tm_mon;
            int today_mday = t_now->tm_mday;
            
            bool hasEvent = false;
            for (const auto& ev : calendarEvents) {
                // Note: Must save today's date first because gmtime uses static buffer!
                // We already saved today_year/mon/mday above.
                struct tm *evTm = gmtime(&ev.start_time);
                if (evTm->tm_year == today_year && 
                    evTm->tm_mon == today_mon && 
                    evTm->tm_mday == today_mday) {
                    hasEvent = true;
                    break;
                }
            }
            
            if (hasEvent) {
                  drawIconFromProgmem(gImage_bell, timeX - 27, 31, 20, 20, 1);
             }
            
            if (solarDate.length() > 0) {
                u8g2.setFont(u8g2_font_wqy12_t_gb2312);
                String fullDate = solarDate;
                if (weekDay.length() > 0) fullDate += " " + weekDay;
                int sdWidth = u8g2.getUTF8Width(fullDate.c_str());
                u8g2.drawUTF8(100 - (sdWidth / 2), 90, fullDate.c_str());
                
                u8g2.setFont(u8g2_font_wqy12_t_gb2312);
                int ldWidth = u8g2.getUTF8Width(lunarDate.c_str());
                u8g2.drawUTF8(100 - (ldWidth / 2), 115, lunarDate.c_str());
                
                int tiWidth = u8g2.getUTF8Width(termInfo.c_str());
                u8g2.drawUTF8(100 - (tiWidth / 2), 135, termInfo.c_str());
            }
        } else {
            // --- DATE MODE ---
            // 1. Display Date in a stylized box: Rounded corners, Split colors
            time_t now = timeClient.getEpochTime();
            struct tm *t_now = gmtime(&now);
            
            // Check for today's event and draw bell icon
            int today_year = t_now->tm_year;
            int today_mon = t_now->tm_mon;
            int today_mday = t_now->tm_mday;
            
            bool hasEvent = false;
            for (const auto& ev : calendarEvents) {
                struct tm *evTm = gmtime(&ev.start_time);
                if (evTm->tm_year == today_year && 
                    evTm->tm_mon == today_mon && 
                    evTm->tm_mday == today_mday) {
                    hasEvent = true;
                    break;
                }
            }
            
            int boxW = 150;
            int boxH = 68;
            int boxX = 100 - (boxW / 2);
            int boxY = 5; // Moved box up (was 15)
            int cornerR = 10; // ~2mm

            if (hasEvent) {
                // Draw bell icon to the left of the box
                drawIconFromProgmem(gImage_bell, boxX - 23, boxY + 15, 20, 20, 1);
            }

            // Draw stylized box with a single outer border and split colors
            // 1. Fill entire box background with white (for the right half)
            paint_gfx.fillRoundRect(boxX, boxY, boxW, boxH, cornerR, 0);
            
            // 2. Fill left half black, but flatten its right edge
            paint_gfx.fillRoundRect(boxX, boxY, boxW / 2, boxH, cornerR, 1);
            paint_gfx.fillRect(boxX + (boxW / 2) - cornerR, boxY, cornerR, boxH, 1);
            
            // 3. Draw the single outer border for the entire box
            paint_gfx.drawRoundRect(boxX, boxY, boxW, boxH, cornerR, 1);
            
            // 4. Draw the center divider line
            paint_gfx.drawFastVLine(boxX + (boxW / 2), boxY, boxH, 1);

            // Draw Text (Month on Left, Day on Right)
            u8g2.setFont(u8g2_font_logisoso50_tf);
            int ascent = u8g2.getFontAscent();
            int textY = boxY + (boxH + ascent) / 2; // Centered for the new font
            
            // Month: White on Black
            char monthStr[5];
            sprintf(monthStr, "%02d", t_now->tm_mon + 1);
            int mWidth = u8g2.getUTF8Width(monthStr);
            u8g2.setForegroundColor(0); // White
            u8g2.setBackgroundColor(1); // Black
            u8g2.drawUTF8(boxX + (boxW / 4) - (mWidth / 2), textY, monthStr);

            // Day: Black on White
            char dayStr[5];
            sprintf(dayStr, "%02d", t_now->tm_mday);
            int dWidth = u8g2.getUTF8Width(dayStr);
            u8g2.setForegroundColor(1); // Black
            u8g2.setBackgroundColor(0); // White
            u8g2.drawUTF8(boxX + (3 * boxW / 4) - (dWidth / 2), textY, dayStr);
            
            // Reset colors for subsequent drawing
            u8g2.setForegroundColor(1);
            u8g2.setBackgroundColor(0);

            // 2. Display Weekday (Icon), Lunar Date, and Term Info in original positions
            // Weekday Icon at y=72 (Slightly overlap/close to box)
            if (weekDay.length() > 0) {
                // Determine which icon to use based on weekDay
                const unsigned char* weekIcon = NULL;
                if (weekDay.indexOf("一") >= 0 || weekDay.indexOf("Mon") >= 0) weekIcon = gImage_mon;
                else if (weekDay.indexOf("二") >= 0 || weekDay.indexOf("Tue") >= 0) weekIcon = gImage_tue;
                else if (weekDay.indexOf("三") >= 0 || weekDay.indexOf("Wed") >= 0) weekIcon = gImage_wed;
                else if (weekDay.indexOf("四") >= 0 || weekDay.indexOf("Thu") >= 0) weekIcon = gImage_thu;
                else if (weekDay.indexOf("五") >= 0 || weekDay.indexOf("Fri") >= 0) weekIcon = gImage_fri;
                else if (weekDay.indexOf("六") >= 0 || weekDay.indexOf("Sat") >= 0) weekIcon = gImage_sat;
                else if (weekDay.indexOf("日") >= 0 || weekDay.indexOf("Sun") >= 0) weekIcon = gImage_sun;
                
                if (weekIcon) {
                    // Draw icon (200x36)
                    // Centering in 200px panel: 100 - (200/2) = 0.
                    drawIconFromProgmem(weekIcon, 50, 81, 200, 36, 1);
                } else {
                    // Fallback to text if icon not found
                    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
                    int wWidth = u8g2.getUTF8Width(weekDay.c_str());
                    u8g2.drawUTF8(100 - (wWidth / 2), 85, weekDay.c_str());
                }
            }

            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            // Restore Lunar Date and Term Info to original positions
            if (lunarDate.length() > 0) {
                int ldWidth = u8g2.getUTF8Width(lunarDate.c_str());
                u8g2.drawUTF8(100 - (ldWidth / 2), 115, lunarDate.c_str());
            }

            if (termInfo.length() > 0) {
                int tiWidth = u8g2.getUTF8Width(termInfo.c_str());
                u8g2.drawUTF8(100 - (tiWidth / 2), 135, termInfo.c_str());
            }
        }

        // === RIGHT SIDE (Today's Weather) ===
        if (currentForecastCount > 0) {
            int iconX = 210;    
            int textCenterX = 340; // 用于温度显示的中心
            int panelCenterX = 300; // 整个右侧面板的中心 (200-400)
            
            int currentHour = timeClient.getHours();
            bool isNight = (currentHour >= config.day_end_hour || currentHour < config.day_start_hour);
            String iconCode = isNight ? currentForecast[0].icon_night : currentForecast[0].icon_day;
            String condText = isNight ? currentForecast[0].cond_night : currentForecast[0].cond_day;
            if (iconCode.length() == 0) iconCode = currentForecast[0].icon_day;
            if (condText.length() == 0) condText = currentForecast[0].cond_day;
            
            // 1. 图标显示逻辑
            bool iconDrawn = false;
            if (iconCode.length() > 0) {
                // Check for Large icon first (-L)
                String iconPathL = "/icons/" + iconCode + "-L.bmp";
                if (LittleFS.exists(iconPathL)) {
                    drawBmp(iconPathL, iconX, 25, 1); 
                    iconDrawn = true;
                } else {
                    String iconPath = "/icons/" + iconCode + ".bmp";
                    if (LittleFS.exists(iconPath)) {
                        drawBmp(iconPath, iconX, 25, 2); 
                        iconDrawn = true;
                    }
                }
            }
            // 2. Try icons.h (Progmem)
            if (!iconDrawn && iconCode.length() > 0) {
                 // Try Large icon first
                 const unsigned char* iconDataL = getIconData(iconCode + "-L");
                 if (iconDataL) {
                     // Assume Large icon is 72x72 (User specified)
                     drawIconFromProgmem(iconDataL, iconX - 4, 25, 72, 72, 1); 
                     iconDrawn = true;
                 } else {
                     // Fallback to standard icon
                     const unsigned char* iconData = getIconData(iconCode);
                     if (iconData) {
                         // Data is 36x36, render full size to avoid cropping
                         drawIconFromProgmem(iconData, iconX, 25, 36, 36, 2); // Scale 2x (72x72)
                         iconDrawn = true;
                     }
                 }
            }
            if (!iconDrawn) {
                u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
                char iconStr[2] = {getIconChar(condText), 0};
                u8g2.drawUTF8(iconX + 12, 73, iconStr);
            }
            
            // 2. 温度显示 (高温/低温)
            String tempStr = currentForecast[0].temp;
            int slashIndex = tempStr.indexOf('/');
            u8g2.setFont(u8g2_font_logisoso42_tf); 
            if (slashIndex > 0) {
                String highTemp = tempStr.substring(0, slashIndex);
                String lowTemp = tempStr.substring(slashIndex + 1);
                int sx1 = textCenterX - 5, sy1 = 63, sx2 = textCenterX + 15, sy2 = 30;
                int thickness = 4; 
                for(int k = -(thickness/2); k < (thickness/2); k++) {
                      paint_gfx.drawLine(sx1 + k, sy1, sx2 + k, sy2, 1);
                }
                int hW = u8g2.getUTF8Width(highTemp.c_str());
                u8g2.drawUTF8(sx1 - hW + 4, 50, highTemp.c_str()); 
                u8g2.drawUTF8(sx2 - 14, 95, lowTemp.c_str());
            } else {
                 int tW = u8g2.getUTF8Width(tempStr.c_str());
                 u8g2.drawUTF8(textCenterX - (tW / 2), 70, tempStr.c_str());
            }
            
            // 3. 合并：天气文本 + 风向风速 (同一行)
            String windDirPart = "";
            if (currentForecast[0].wind_dir.length() > 0) windDirPart = currentForecast[0].wind_dir;
            
            String windScPart = "";
            String jiPart = "";
            if (currentForecast[0].wind_sc.length() > 0) {
                windScPart = currentForecast[0].wind_sc;
                jiPart = "级";
            }
            
            // 构建第一部分：天气 + 空格 + 风向 + 风速数值
            String weatherDetailPart1 = condText + "  " + windDirPart + windScPart;

            u8g2.setFont(u8g2_font_wqy12_t_gb2312); // Use wqy12
            int wdW1 = u8g2.getUTF8Width(weatherDetailPart1.c_str());
            int wdW2 = (jiPart.length() > 0) ? u8g2.getUTF8Width(jiPart.c_str()) : 0;
            
            // Air Quality Part
            String aqPart = "";
            if (airCategory.length() > 0) {
                 aqPart = "  " + airCategory; 
            }
            int wdW3 = (aqPart.length() > 0) ? u8g2.getUTF8Width(aqPart.c_str()) : 0;

            // 增加3像素间距
            int gap = (jiPart.length() > 0) ? 3 : 0;
            int totalW = wdW1 + gap + wdW2 + wdW3;
            
            int wdX = panelCenterX - (totalW / 2); // 以右侧面板中心居中
            int wdY = 115; // Align with lunar date (y=120 -> 115)
            
            // 绘制第一部分 
            u8g2.drawUTF8(wdX, wdY, weatherDetailPart1.c_str());
            u8g2.drawUTF8(wdX, wdY, weatherDetailPart1.c_str());
            
            // 绘制第二部分 "级"
            if (jiPart.length() > 0) {
                u8g2.drawUTF8(wdX + wdW1 + gap, wdY, jiPart.c_str());
                u8g2.drawUTF8(wdX + wdW1 + gap + 1, wdY, jiPart.c_str());
            }

            // 绘制第三部分 Air Quality (正常粗细)
            if (aqPart.length() > 0) {
                u8g2.drawUTF8(wdX + wdW1 + gap + wdW2, wdY, aqPart.c_str());
            }

            // 4. 室内环境 (基准行) / 或 DATE 模式下的时间戳
            if (config.ui_mode == 0) {
                if (indoorTemp.length() > 0) {
                     u8g2.setFont(u8g2_font_wqy12_t_gb2312); // Use wqy12
                     
                     // 分割为两部分，在%前增加间距
                     String envPart1 = "T:" + indoorTemp + "°C  H:" + indoorHumi;
                     String envPart2 = "%";
                     
                     int inW1 = u8g2.getUTF8Width(envPart1.c_str());
                     int inW2 = u8g2.getUTF8Width(envPart2.c_str());
                     int inGap = 3;
                     
                     int totalInW = inW1 + inGap + inW2;
                     int inX = panelCenterX - (totalInW / 2);
                     int inY = 135; // Align with solar term (y=140 -> 135)
                     
                     // 垂直对齐：固定在底部 153 位置
                     u8g2.drawUTF8(inX, inY, envPart1.c_str()); 
                     u8g2.drawUTF8(inX + inW1 + inGap, inY, envPart2.c_str());
                }
            } else {
                // DATE Mode: Show Update Timestamp instead of Indoor Temp/Humi
                u8g2.setFont(u8g2_font_wqy12_t_gb2312);
                time_t now = timeClient.getEpochTime();
                struct tm *t_now = gmtime(&now);
                char timeStr[25];
                sprintf(timeStr, "update: %02d-%02d %02d:%02d", t_now->tm_mon + 1, t_now->tm_mday, t_now->tm_hour, t_now->tm_min);
                int utW = u8g2.getUTF8Width(timeStr);
                int inY = 135;
                u8g2.drawUTF8(panelCenterX - (utW / 2), inY, timeStr);
            }
        }
        
        // 分割线
        paint_gfx.drawFastHLine(0, 155, EPD_4IN2_WIDTH, 2); 

        // === 底部 6 天预报 ===
        int startY = 155;
        
        // 1. Pre-calculate Min/Max for scaling
        int minTemp = 100, maxTemp = -100;
        int highs[7], lows[7];
        int xCoords[7];
        int count = 0;

        for(int i=1; i<currentForecastCount && i<=6; i++) {
            String tStr = currentForecast[i].temp;
            int slash = tStr.indexOf('/');
            if (slash > 0) {
                highs[i] = tStr.substring(0, slash).toInt();
                lows[i] = tStr.substring(slash + 1).toInt();
            } else {
                highs[i] = tStr.toInt();
                lows[i] = tStr.toInt();
            }
            if (highs[i] > maxTemp) maxTemp = highs[i];
            if (lows[i] < minTemp) minTemp = lows[i];
            if (highs[i] < minTemp) minTemp = highs[i]; // Safety
            if (lows[i] > maxTemp) maxTemp = lows[i];   // Safety
            count++;
        }
        
        // Add padding to range
        if (maxTemp == minTemp) { maxTemp++; minTemp--; }
        int tempRange = maxTemp - minTemp;
        if (tempRange == 0) tempRange = 1; // Prevent div/0

        // Chart Area
        int chartTop = startY + 60;   
        int chartBottom = startY + 90; 
        int chartHeight = chartBottom - chartTop;

        for(int i=1; i<currentForecastCount && i<=6; i++) {
            int x1 = (i - 1) * EPD_4IN2_WIDTH / 6;
            int x2 = i * EPD_4IN2_WIDTH / 6;
            int centerX = x1 + ((x2 - x1) / 2);
            xCoords[i] = centerX;
            
            // 0. Date (Added above day icon)
            String dateShort = currentForecast[i].date;
            if (dateShort.length() >= 10) dateShort = dateShort.substring(5);
            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            int dWidth = u8g2.getUTF8Width(dateShort.c_str());
            u8g2.drawUTF8(centerX - (dWidth/2), startY , dateShort.c_str());

            // Day Icon (Moved further DOWN to accommodate date)
            // Was startY + 2. Date ends at startY + 12. Icon needs space.
            // Let's move icon to startY + 16.
            int iconY = startY + 16;
            
            bool dayIconDrawn = false;
            if (currentForecast[i].icon_day.length() > 0) {
                String iconPath = "/icons/" + currentForecast[i].icon_day + ".bmp";
                if (LittleFS.exists(iconPath)) {
                    drawBmp(iconPath, centerX - 16, iconY); 
                    dayIconDrawn = true;
                }
            }
            if (!dayIconDrawn && currentForecast[i].icon_day.length() > 0) {
                 const unsigned char* iconData = getIconData(currentForecast[i].icon_day);
                 if (iconData) drawIconFromProgmem(iconData, centerX - 18, iconY -7, 36, 36, 1); 
                 dayIconDrawn = true;
            }
            
            // Night Icon (Moved DOWN to startY + 102)
            bool nightIconDrawn = false;
            if (currentForecast[i].icon_night.length() > 0) {
                String iconPath = "/icons/" + currentForecast[i].icon_night + ".bmp";
                if (LittleFS.exists(iconPath)) {
                     drawBmp(iconPath, centerX - 16, startY + 104);   
                     nightIconDrawn = true;
                }
            }
            
            if (!nightIconDrawn && currentForecast[i].icon_night.length() > 0) {
                 const unsigned char* iconData = getIconData(currentForecast[i].icon_night);
                 if (iconData) {
                     drawIconFromProgmem(iconData, centerX - 18, startY + 104, 36, 36, 1); 
                     nightIconDrawn = true;
                 }
            }

            if (!nightIconDrawn) {
                u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);
                char nightIconStr[2] = {getIconChar(currentForecast[i].cond_night), 0};
                u8g2.drawUTF8(centerX - 16, startY + 132, nightIconStr); // 104+28 approx
            }

            // Separator line removed
        }
        
        // 4. Draw Curves
        if (count > 1) {
            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            for (int i = 1; i < currentForecastCount && i < 6; i++) { // Connect i to i+1
                 // Map High
                 int yH1 = chartBottom - ((highs[i] - minTemp) * chartHeight / tempRange);
                 int yH2 = chartBottom - ((highs[i+1] - minTemp) * chartHeight / tempRange);
                 paint_gfx.drawLine(xCoords[i], yH1, xCoords[i+1], yH2, 1);
                 
                 // Map Low
                 int yL1 = chartBottom - ((lows[i] - minTemp) * chartHeight / tempRange);
                 int yL2 = chartBottom - ((lows[i+1] - minTemp) * chartHeight / tempRange);
                 paint_gfx.drawLine(xCoords[i], yL1, xCoords[i+1], yL2, 1);
                 
                 // Draw dots and text for point i
                 paint_gfx.fillCircle(xCoords[i], yH1, 2, 1);
                 paint_gfx.fillCircle(xCoords[i], yL1, 2, 1);
                 
                 String hStr = String(highs[i]);
                 int hw = u8g2.getUTF8Width(hStr.c_str());
                 u8g2.drawUTF8(xCoords[i] - hw/2, yH1 - 5, hStr.c_str());
                 
                 String lStr = String(lows[i]);
                 int lw = u8g2.getUTF8Width(lStr.c_str());
                 u8g2.drawUTF8(xCoords[i] - lw/2, yL1 + 12, lStr.c_str());
                 
                 // Handle last point (i+1) in the last iteration
                 if (i == count - 1 || i == 5) {
                     paint_gfx.fillCircle(xCoords[i+1], yH2, 2, 1);
                     paint_gfx.fillCircle(xCoords[i+1], yL2, 2, 1);
                     
                     String hStr2 = String(highs[i+1]);
                     int hw2 = u8g2.getUTF8Width(hStr2.c_str());
                     u8g2.drawUTF8(xCoords[i+1] - hw2/2, yH2 - 5, hStr2.c_str());
                     
                     String lStr2 = String(lows[i+1]);
                     int lw2 = u8g2.getUTF8Width(lStr2.c_str());
                     u8g2.drawUTF8(xCoords[i+1] - lw2/2, yL2 + 12, lStr2.c_str());
                 }
            }
        }

        // 刷屏控制
        if (partial_update) {
            Local_EPD_4IN2_Init_Partial();
            Local_EPD_4IN2_PartialDisplay(0, 0, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, BlackImage);
        } else {
            Local_EPD_4IN2_Init();
            Local_EPD_4IN2_Display(BlackImage);
        }
        hibernateEPD();
        
        // Signal completion via Serial2 ONLY if requested (weather MQTT updates)
        if (sendSignal) {
            Serial2.println("bye");
            digitalWrite(BYE_SIGNAL_PIN, LOW); // Trigger low level signal
            Serial.println("Sent 'bye' via Serial2 and BYE_SIGNAL_PIN set to LOW");
        }
        
        // free(BlackImage); // Keep allocated
        // BlackImage = NULL;

    }
}

void hibernateEPD() {
    Serial.println("Hibernating EPD...");
    // 1. Put EPD to deep sleep mode via its controller
    Local_EPD_4IN2_Sleep();
    
    // 2. Disable all possible EPD pins to prevent leakage
    pinMode(EPD_SCK_PIN, INPUT);
    pinMode(EPD_MOSI_PIN, INPUT);
    pinMode(EPD_CS_PIN, INPUT);
    pinMode(EPD_RST_PIN, INPUT);
    pinMode(EPD_DC_PIN, INPUT);
    pinMode(EPD_BUSY_PIN, INPUT);
    Serial.println("EPD pins isolated.");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] Length: ");
  Serial.println(length);
  
  if (strcmp(topic, config.mqtt_unified_topic) == 0) {
      Serial.println("Unified message received");
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, (char*)payload, length);
      if (!error) {
          // 1. Parse Date Info
          if (doc.containsKey("date")) {
              JsonObject dateObj = doc["date"];
              if (dateObj.containsKey("solar")) solarDate = dateObj["solar"].as<String>();
              if (dateObj.containsKey("week")) weekDay = dateObj["week"].as<String>();
              if (dateObj.containsKey("lunar")) lunarDate = dateObj["lunar"].as<String>();
              if (dateObj.containsKey("term")) termInfo = dateObj["term"].as<String>();
              updateDatePending = true;
          }
          
          // 2. Parse Weather Info
          if (doc.containsKey("weather")) {
              JsonArray forecastArr = doc["weather"].as<JsonArray>();
              int count = 0;
              for (JsonVariant v : forecastArr) {
                  if (count >= 7) break;
                  
                  if (v.containsKey("temp")) currentForecast[count].temp = v["temp"].as<String>();
                  
                  // Icon Day
                  if (v.containsKey("icon")) currentForecast[count].icon_day = v["icon"].as<String>();
                  else if (v.containsKey("iconDay")) currentForecast[count].icon_day = v["iconDay"].as<String>();
                  else if (v.containsKey("icon_day")) currentForecast[count].icon_day = v["icon_day"].as<String>();
                  
                  // Icon Night
                  if (v.containsKey("iconNight")) currentForecast[count].icon_night = v["iconNight"].as<String>();
                  else if (v.containsKey("icon_night")) currentForecast[count].icon_night = v["icon_night"].as<String>();

                  // Condition Day
                  if (v.containsKey("cond")) currentForecast[count].cond_day = v["cond"].as<String>();
                  else if (v.containsKey("textDay")) currentForecast[count].cond_day = v["textDay"].as<String>();
                  else if (v.containsKey("text")) currentForecast[count].cond_day = v["text"].as<String>();
                  else if (v.containsKey("weather")) currentForecast[count].cond_day = v["weather"].as<String>();
                  
                  // Condition Night
                  if (v.containsKey("textNight")) currentForecast[count].cond_night = v["textNight"].as<String>();
                  else if (v.containsKey("text_night")) currentForecast[count].cond_night = v["text_night"].as<String>();
                  else if (v.containsKey("cond_night")) currentForecast[count].cond_night = v["cond_night"].as<String>();
                  
                  if (v.containsKey("windDir")) currentForecast[count].wind_dir = v["windDir"].as<String>();
                  if (v.containsKey("windScale")) currentForecast[count].wind_sc = v["windScale"].as<String>();
                  if (v.containsKey("date")) currentForecast[count].date = v["date"].as<String>();
                  
                  count++;
              }
              currentForecastCount = count;
              updateWeatherPending = true;
              fullRefreshPending = true; // Set full refresh flag for weather data
          }
          
          // 3. Parse Indoor Environment
          if (doc.containsKey("env")) {
              JsonObject envObj = doc["env"];
              if (envObj.containsKey("temp")) indoorTemp = envObj["temp"].as<String>();
              if (envObj.containsKey("humi")) indoorHumi = envObj["humi"].as<String>();
              updateEnvPending = true;
          }
          
          // 4. Parse Air Quality
          if (doc.containsKey("air")) {
              JsonObject airObj = doc["air"];
              if (airObj.containsKey("pm2p5")) airPm2p5 = airObj["pm2p5"].as<String>();
              if (airObj.containsKey("category")) airCategory = airObj["category"].as<String>();
              updateWeatherPending = true;
          }
          
          lastUpdateTrigger = millis();
          Serial.println("Unified update processed");
      } else {
          Serial.print("JSON Error (Unified): ");
          Serial.println(error.c_str());
          displayMessage("Unified JSON Error:\n" + String(error.c_str()));
      }
      return;
  }

  if (strcmp(topic, config.mqtt_date_topic) == 0) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, (char*)payload, length);
      if (!error) {
          if (doc.containsKey("阳历日期")) solarDate = doc["阳历日期"].as<String>();
          if (doc.containsKey("星期")) weekDay = doc["星期"].as<String>();
          if (doc.containsKey("农历日期")) lunarDate = doc["农历日期"].as<String>();
          if (doc.containsKey("节气信息")) termInfo = doc["节气信息"].as<String>();
          
          Serial.println("Date info updated");
          updateDatePending = true;
          lastUpdateTrigger = millis();
      } else {
          Serial.print("JSON Error (Date): ");
          Serial.println(error.c_str());
      }
      return;
  }

        if (strcmp(topic, config.mqtt_env_topic) == 0) {
            Serial.println("Env updated");
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (char*)payload, length);
            if (!error) {
                if (doc.containsKey("temp")) indoorTemp = doc["temp"].as<String>();
                if (doc.containsKey("humi")) indoorHumi = doc["humi"].as<String>();
                
                updateEnvPending = true;
                lastUpdateTrigger = millis();
            } else {
                Serial.print("JSON Error (Env): ");
                Serial.println(error.c_str());
            }
            return;
        }

        if (strcmp(topic, config.mqtt_air_quality_topic) == 0) {
             Serial.println("Air Quality Message Received");
             JsonDocument doc;
             DeserializationError error = deserializeJson(doc, payload, length);
             if (!error) {
                 if (doc.containsKey("pm2p5")) airPm2p5 = doc["pm2p5"].as<String>();
                 if (doc.containsKey("category")) airCategory = doc["category"].as<String>();
                 updateWeatherPending = true; // Refresh weather page to show new air quality
                 lastUpdateTrigger = millis();
                 Serial.printf("Air Quality Updated: PM2.5=%s, Category=%s\n", airPm2p5.c_str(), airCategory.c_str());
             } else {
                 Serial.print("JSON Error (Air): ");
                 Serial.println(error.c_str());
             }
             return;
        }

        if (strcmp(topic, config.mqtt_shift_topic) == 0) {
      Serial.println("Shift updated");
      Serial.print("Payload length: ");
      Serial.println(length);

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, (char*)payload, length);
      
      if (!error) {
          // Parse into temporary vector first
          std::vector<ShiftEvent> newShifts;
          
          // Use current time as default Year/Month context for parsing simple dates
          time_t now = timeClient.getEpochTime();
          struct tm *t_now = gmtime(&now);
          int currentYear = t_now->tm_year + 1900;
          int currentMonth = t_now->tm_mon + 1;
          
          // Helper lambda to add to temp list
          auto addTempShift = [&](int y, int m, int d, String c) {
              ShiftEvent ev;
              ev.year = y; ev.month = m; ev.day = d; ev.content = c;
              newShifts.push_back(ev);
          };
          
          // 1. Handle Array: ["2026-02-17", "Shift"] or [{"date":"...", "shift":"..."}]
          if (doc.is<JsonArray>()) {
               JsonArray arr = doc.as<JsonArray>();
               if (arr.size() == 2 && arr[0].is<String>() && arr[1].is<String>()) {
                    String dateStr = arr[0].as<String>();
                    int year, month, day;
                    if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                        addTempShift(year, month, day, arr[1].as<String>());
                    }
               } 
               else {
                   for (JsonVariant v : arr) {
                       if (v.is<JsonObject>()) {
                           JsonObject obj = v.as<JsonObject>();
                           String dateStr = "";
                           if (obj.containsKey("date")) dateStr = obj["date"].as<String>();
                           else if (obj.containsKey("day")) dateStr = obj["day"].as<String>();
                           
                           if (dateStr.length() > 0) {
                               int year, month, day;
                               if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                                   String content = "";
                                   if (obj.containsKey("shift")) content = obj["shift"].as<String>();
                                   else if (obj.containsKey("content")) content = obj["content"].as<String>();
                                   else if (obj.containsKey("summary")) content = obj["summary"].as<String>();
                                   
                                   if (content.length() > 0) addTempShift(year, month, day, content);
                               }
                           }
                       }
                   }
               }
          }
          // 2. Handle Object
          else if (doc.is<JsonObject>()) {
               JsonObject root = doc.as<JsonObject>();
               bool foundDateKeys = false;
               for (JsonPair kv : root) {
                   String rawKey = kv.key().c_str();
                   String key = "";
                   for (unsigned int i = 0; i < rawKey.length(); i++) {
                       char c = rawKey[i];
                       if (isdigit(c) || c == '-') key += c;
                   }

                   int year, month, day;
                   if (sscanf(key.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                       if (kv.value().is<String>()) {
                           String content = kv.value().as<String>();
                           if (content.length() > 0) {
                               addTempShift(year, month, day, content);
                               foundDateKeys = true;
                               currentYear = year;
                               currentMonth = month;
                           }
                       }
                   } 
                   else if (sscanf(key.c_str(), "%d", &day) == 1) {
                       if (key.indexOf('-') == -1 && day >= 1 && day <= 31 && kv.value().is<String>()) {
                           String content = kv.value().as<String>();
                           if (content.length() > 0) {
                               addTempShift(currentYear, currentMonth, day, content);
                               foundDateKeys = true;
                           }
                       }
                   }
               }
               
               if (!foundDateKeys) {
                   if (root.containsKey("date") || root.containsKey("day")) {
                        String dateStr = root.containsKey("date") ? root["date"].as<String>() : root["day"].as<String>();
                        int year, month, day;
                        if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                            String content = "";
                            if (root.containsKey("shift")) content = root["shift"].as<String>();
                            else if (root.containsKey("content")) content = root["content"].as<String>();
                            if (content.length() > 0) addTempShift(year, month, day, content);
                        }
                   }
                   else {
                       JsonArray shifts;
                       if (root.containsKey("shifts")) shifts = root["shifts"];
                       else if (root.containsKey("data")) shifts = root["data"];
                       
                       if (!shifts.isNull()) {
                           for (JsonVariant v : shifts) {
                               if (v.is<JsonObject>()) {
                                   JsonObject obj = v.as<JsonObject>();
                                   String dateStr = "";
                                   if (obj.containsKey("date")) dateStr = obj["date"].as<String>();
                                   
                                   int year, month, day;
                                   if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                                       String content = "";
                                       if (obj.containsKey("shift")) content = obj["shift"].as<String>();
                                       if (content.length() > 0) addTempShift(year, month, day, content);
                                   }
                               }
                           }
                       }
                   }
               }
          }

          // MERGE LOGIC: 
          // 1. Find min date in newShifts
          // 2. Remove all existing shifts >= min date
          // 3. Add all new shifts
          
          if (!newShifts.empty()) {
              // Find min date
              ShiftEvent minEv = newShifts[0];
              for(const auto& ev : newShifts) {
                  if (ev.year < minEv.year) minEv = ev;
                  else if (ev.year == minEv.year && ev.month < minEv.month) minEv = ev;
                  else if (ev.year == minEv.year && ev.month == minEv.month && ev.day < minEv.day) minEv = ev;
              }
              
              // Remove existing >= minDate
              auto it = std::remove_if(shiftEvents.begin(), shiftEvents.end(), [&](const ShiftEvent& ev) {
                  if (ev.year > minEv.year) return true;
                  if (ev.year == minEv.year && ev.month > minEv.month) return true;
                  if (ev.year == minEv.year && ev.month == minEv.month && ev.day >= minEv.day) return true;
                  return false;
              });
              shiftEvents.erase(it, shiftEvents.end());
              
              // Add new shifts
              shiftEvents.insert(shiftEvents.end(), newShifts.begin(), newShifts.end());
              
              Serial.printf("Merged %d new shifts. Min Date: %d-%d-%d\n", newShifts.size(), minEv.year, minEv.month, minEv.day);
          }
          
          // Sort events by date
          std::sort(shiftEvents.begin(), shiftEvents.end(), [](const ShiftEvent& a, const ShiftEvent& b) {
              if (a.year != b.year) return a.year < b.year;
              if (a.month != b.month) return a.month < b.month;
              return a.day < b.day;
          });
          
          // Enforce Max Size (Remove Oldest)
          while (shiftEvents.size() > MAX_SHIFT_EVENTS) {
              shiftEvents.erase(shiftEvents.begin());
          }
          
          Serial.print("Total shifts stored: ");
          Serial.println(shiftEvents.size());
          
          updateCalendarPending = true;
          lastUpdateTrigger = millis();
      } else {
          Serial.print("JSON Error (Shift): ");
          Serial.println(error.c_str());
      }
      return;
  }

  if (strcmp(topic, config.mqtt_calendar_topic) == 0) {
      Serial.println("Calendar updated");

      JsonDocument doc;
      // Filter to reduce memory usage
      JsonDocument filter;
      filter["events"][0]["start"] = true;
      filter["events"][0]["summary"] = true;
      // Support 1 level nesting
      filter["*"]["events"][0]["start"] = true;
      filter["*"]["events"][0]["summary"] = true;
      
      DeserializationError error = deserializeJson(doc, (char*)payload, length, DeserializationOption::Filter(filter));
      if (!error) {
          calendarEvents.clear();
          // The structure is {"calendar.my_calendar":{"events":[...]}}
          // We need to find the events array. It might be nested or direct.
          
          JsonArray events;
          if (doc.containsKey("events")) {
              events = doc["events"];
          } else {
              // Iterate through keys to find one that contains "events"
              JsonObject root = doc.as<JsonObject>();
              for (JsonPair kv : root) {
                   if (kv.value().is<JsonObject>() && kv.value().as<JsonObject>().containsKey("events")) {
                       events = kv.value()["events"];
                       break;
                   }
              }
          }

          if (!events.isNull()) {
              for (JsonVariant v : events) {
                  if (calendarEvents.size() >= MAX_CALENDAR_EVENTS) break; // Limit events

                  CalendarEvent ev;
                  // Handle "start" time string: "2026-02-15T23:00:00+08:00"
                  String startStr = v["start"].as<String>();
                  // Simple parsing for now (assuming fixed format or using library if available)
                  // For this demo, let's parse basic components
                  // 2026-02-15T23:00:00+08:00
                  int year = startStr.substring(0, 4).toInt();
                  int month = startStr.substring(5, 7).toInt();
                  int day = startStr.substring(8, 10).toInt();
                  int hour = startStr.substring(11, 13).toInt();
                  int minute = startStr.substring(14, 16).toInt();
                  int second = startStr.substring(17, 19).toInt();
                  
                  struct tm t = {0};
                   t.tm_year = year - 1900;
                   t.tm_mon = month - 1;
                   t.tm_mday = day;
                   t.tm_hour = hour;
                   t.tm_min = minute;
                   t.tm_sec = second;
                   t.tm_isdst = -1;
                   
                   // mktime uses local time zone, but we don't have TZ set in environment usually.
                   // However, timeClient uses an offset.
                   // The date string IS local time (UTC+8).
                   // We want to compare this with "now" from timeClient which is also shifted by offset.
                   // So we should treat this struct tm as UTC so mktime returns the raw epoch that represents that specific time point 
                   // BUT wait, timeClient.getEpochTime() returns UTC epoch + offset?
                   // No, getEpochTime() returns Unix Epoch (UTC).
                   // But getHours() uses the offset.
                   // Wait, timeClient documentation says:
                   // "getEpochTime() returns the Unix epoch, which is the number of seconds that have elapsed since January 1, 1970 (midnight UTC/GMT), not counting leap seconds."
                   // BUT we initialize it with offset 28800 (UTC+8).
                   // Let's check NTPClient source or assume:
                   // If we init with offset, getEpochTime() returns (UTC_Epoch + Offset).
                   // So it returns "Local Epoch".
                   
                   // So if we parse "2026-02-15 23:00" which IS local time.
                   // We want to convert this YMDHMS (Local) to "Local Epoch".
                   // Since ESP8266 default mktime assumes UTC (if TZ not set),
                   // mktime(&t) will give us the epoch AS IF that time was UTC.
                   // This is exactly what we want! 
                   // Example: 
                   // Real UTC: 12:00. Local (UTC+8): 20:00.
                   // timeClient (with offset) returns epoch for 20:00.
                   // Input string "20:00".
                   // mktime("20:00") -> epoch for 20:00.
                   // So they match!
                   
                   ev.start_time = mktime(&t); 
                   
                   // Debug
                   Serial.print("Event: ");
                   Serial.print(year); Serial.print("-"); Serial.print(month); Serial.print("-"); Serial.print(day);
                   Serial.print(" -> Epoch: "); Serial.println(ev.start_time);
                   
                   ev.summary = v["summary"].as<String>();
                   calendarEvents.push_back(ev);
              }
              Serial.print("Parsed events: ");
              Serial.println(calendarEvents.size());
              
              updateCalendarPending = true;
              lastUpdateTrigger = millis();
          }
      } else {
          Serial.print("JSON Error (Calendar): ");
          Serial.println(error.c_str());
      }
      return;
  }
  
  if (strcmp(topic, "epd/weatherrequest") == 0) {
       // Ignore our own request messages if subscribed
       return; 
  }

  if (strcmp(topic, config.mqtt_weather_topic) == 0) {
      Serial.println("Weather updated");

      JsonDocument doc;
      // Filter to exclude hourly/minutely data which consumes memory
      JsonDocument filter;
      // Array root
      filter[0]["fxDate"] = true; filter[0]["date"] = true; filter[0]["日期"] = true;
      filter[0]["tempMax"] = true; filter[0]["tempMin"] = true; filter[0]["temp"] = true; filter[0]["最高温"] = true; filter[0]["最低温"] = true;
      filter[0]["textDay"] = true; filter[0]["textNight"] = true; filter[0]["白天天气"] = true; filter[0]["夜晚天气"] = true; filter[0]["天气"] = true; filter[0]["weather"] = true;
      filter[0]["iconDay"] = true; filter[0]["iconNight"] = true;
      filter[0]["windDirDay"] = true; filter[0]["windDir"] = true; filter[0]["风向"] = true;
      filter[0]["windScaleDay"] = true; filter[0]["windScale"] = true; filter[0]["windSpeedDay"] = true; filter[0]["风速"] = true;
      
      // Nested arrays
      filter["daily"] = filter; // Reuse the array filter structure for "daily" key
      filter["data"] = filter;
      filter["forecast"] = filter;
      
      // But wait, filter["daily"] = filter assigns the WHOLE filter object to "daily".
      // The filter object currently contains [0]... and "daily"...
      // This recursive assignment might be weird or efficient.
      // Actually, if I do `filter["daily"][0]["fxDate"] = true`, it's better.
      // But re-typing is tedious.
      // Let's just type it out to be safe and clear.
      
      // Re-doing the filter construction properly:
      JsonDocument f;
      JsonObject p = f.to<JsonObject>();
      // Define the element filter
      JsonObject e = p.createNestedObject("e"); // Temporary object to hold element filter
      e["fxDate"] = true; e["date"] = true; e["日期"] = true;
      e["tempMax"] = true; e["tempMin"] = true; e["temp"] = true; e["最高温"] = true; e["最低温"] = true;
      e["textDay"] = true; e["textNight"] = true; e["白天天气"] = true; e["夜晚天气"] = true; e["天气"] = true; e["weather"] = true;
      e["iconDay"] = true; e["iconNight"] = true;
      e["windDirDay"] = true; e["windDir"] = true; e["风向"] = true;
      e["windScaleDay"] = true; e["windScale"] = true; e["windSpeedDay"] = true; e["风速"] = true;
      
      // Apply to paths
      filter[0] = e;
      filter["daily"][0] = e;
      filter["data"][0] = e;
      filter["forecast"][0] = e;

      DeserializationError error = deserializeJson(doc, (char*)payload, length, DeserializationOption::Filter(filter));

      if (!error) {
          Serial.println("Parsing JSON...");
          
          int count = 0;

          // Check if root is an array (specific format requested)
          if (doc.is<JsonArray>()) {
              Serial.println("Found root array data");
              JsonArray data = doc.as<JsonArray>();
              for(JsonVariant v : data) {
                  if (count >= 7) break;
                  
                  // Mapping new JSON format keys
                  if (v.containsKey("fxDate")) currentForecast[count].date = v["fxDate"].as<String>();
                  else if (v.containsKey("日期")) currentForecast[count].date = v["日期"].as<String>();
                  else if (v.containsKey("date")) currentForecast[count].date = v["date"].as<String>();
                  
                  // Temp: High/Low range
                  if (v.containsKey("tempMax") && v.containsKey("tempMin")) {
                      currentForecast[count].temp = v["tempMax"].as<String>() + "/" + v["tempMin"].as<String>();
                  } else if (v.containsKey("最高温") && v.containsKey("最低温")) {
                      currentForecast[count].temp = v["最高温"].as<String>() + "/" + v["最低温"].as<String>();
                  } else if (v.containsKey("temp")) {
                      currentForecast[count].temp = v["temp"].as<String>();
                  }
                  
                  // Condition Day
                  if (v.containsKey("textDay")) currentForecast[count].cond_day = v["textDay"].as<String>();
                  else if (v.containsKey("白天天气")) currentForecast[count].cond_day = v["白天天气"].as<String>();
                  else if (v.containsKey("天气")) currentForecast[count].cond_day = v["天气"].as<String>();
                  else if (v.containsKey("weather")) currentForecast[count].cond_day = v["weather"].as<String>();
                  
                  // Condition Night
                  if (v.containsKey("textNight")) currentForecast[count].cond_night = v["textNight"].as<String>();
                  else if (v.containsKey("夜晚天气")) currentForecast[count].cond_night = v["夜晚天气"].as<String>();

                  // Icons
                  if (v.containsKey("iconDay")) currentForecast[count].icon_day = v["iconDay"].as<String>();
                  if (v.containsKey("iconNight")) currentForecast[count].icon_night = v["iconNight"].as<String>();
                  
                  // Wind
                  if (v.containsKey("windDirDay")) currentForecast[count].wind_dir = v["windDirDay"].as<String>();
                  else if (v.containsKey("windDir")) currentForecast[count].wind_dir = v["windDir"].as<String>();
                  else if (v.containsKey("风向")) currentForecast[count].wind_dir = v["风向"].as<String>();
                  
                  if (v.containsKey("windScaleDay")) currentForecast[count].wind_sc = v["windScaleDay"].as<String>();
                  else if (v.containsKey("windScale")) currentForecast[count].wind_sc = v["windScale"].as<String>();
                  else if (v.containsKey("windSpeedDay")) currentForecast[count].wind_sc = v["windSpeedDay"].as<String>();
                  else if (v.containsKey("风速")) currentForecast[count].wind_sc = v["风速"].as<String>();
                  
                  if (currentForecast[count].date.length() > 0) {
                      count++;
                  }
              }
          } 
          // Handle object with data array (standard formats)
          else {
              JsonArray data = doc["data"];
              if (data.isNull()) data = doc["daily"]; 
              if (data.isNull()) data = doc["forecast"];
              
              if (!data.isNull()) {
                  Serial.println("Found nested array data");
                  for(JsonVariant v : data) {
                      if (count >= 7) break;
                      
                      if (v.containsKey("date")) currentForecast[count].date = v["date"].as<String>();
                      else if (v.containsKey("day")) currentForecast[count].date = v["day"].as<String>();
                      
                      if (v.containsKey("temp")) currentForecast[count].temp = v["temp"].as<String>();
                      else if (v.containsKey("high")) currentForecast[count].temp = v["high"].as<String>();
                      
                      if (v.containsKey("weather")) currentForecast[count].cond_day = v["weather"].as<String>();
                      else if (v.containsKey("text")) currentForecast[count].cond_day = v["text"].as<String>();
                      
                      if (currentForecast[count].date.length() > 0) count++;
                  }
              }
          }

          Serial.print("Forecast count: ");
          Serial.println(count);
          currentForecastCount = count;

          if (count > 0) {
              updateWeatherPending = true;
              fullRefreshPending = true; // Set full refresh flag for weather data
              lastUpdateTrigger = millis();
          } else {
               String prettyJson;
               serializeJsonPretty(doc, prettyJson);
               displayMessage("JSON Data (No forecast found):\n" + prettyJson);
          }
      } else {
          Serial.print("JSON Error: ");
          Serial.println(error.c_str());
          
          String msg = "";
          for (int i = 0; i < length; i++) msg += (char)payload[i];
          displayMessage("JSON Error:\n" + String(error.c_str()) + "\n" + msg);
      }
  } else {
      String msg = "";
      for (int i = 0; i < length; i++) msg += (char)payload[i];
      displayMessage(msg);
  }
}

bool reconnect() {
  if (strlen(config.mqtt_server) == 0) return false;
  
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    bool connected = false;
    if (strlen(config.mqtt_user) > 0) {
      connected = client.connect(clientId.c_str(), config.mqtt_user, config.mqtt_pass);
    } else {
      connected = client.connect(clientId.c_str());
    }

    if (connected) {
      Serial.println("connected");
      client.subscribe(config.mqtt_topic);
      if (strlen(config.mqtt_weather_topic) > 0) {
          if (client.subscribe(config.mqtt_weather_topic)) {
              Serial.print("Subscribed to: ");
              Serial.println(config.mqtt_weather_topic);
          } else {
              Serial.println("Subscription failed");
          }
      }
      if (strlen(config.mqtt_date_topic) > 0) {
          if (client.subscribe(config.mqtt_date_topic)) {
              Serial.print("Subscribed to: ");
              Serial.println(config.mqtt_date_topic);
          }
      }
      if (strlen(config.mqtt_env_topic) > 0) {
          if (client.subscribe(config.mqtt_env_topic)) {
              Serial.print("Subscribed to: ");
              Serial.println(config.mqtt_env_topic);
          }
      }
      if (strlen(config.mqtt_calendar_topic) > 0) {
          if (client.subscribe(config.mqtt_calendar_topic)) {
              Serial.print("Subscribed to: ");
              Serial.println(config.mqtt_calendar_topic);
          }
      }
      if (strlen(config.mqtt_shift_topic) > 0) {
          if (client.subscribe(config.mqtt_shift_topic)) {
              Serial.print("Subscribed to: ");
              Serial.println(config.mqtt_shift_topic);
          }
      }
      if (strlen(config.mqtt_air_quality_topic) > 0) {
          if (client.subscribe(config.mqtt_air_quality_topic)) {
              Serial.print("Subscribed to: ");
              Serial.println(config.mqtt_air_quality_topic);
          }
      }
      
      if (strlen(config.mqtt_unified_topic) > 0) {
          if (client.subscribe(config.mqtt_unified_topic)) {
              Serial.print("Subscribed to: ");
              Serial.println(config.mqtt_unified_topic);
          }
      }
      
      // Send Weather Request on Connect
      client.loop();
      delay(100);
      
      if (client.publish("epd/weatherrequest", "get")) {
          Serial.println("Weather Request sent (Reconnect)");
      } else {
          Serial.println("Weather Request failed (Reconnect)");
      }
      return true;

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again later");
      return false;
    }
  }
  return true;
}

// handleRoot moved to WebHandler.cpp

// handleFileManager moved to WebHandler.cpp

// WebHandler functions now in separate file

// Button Handlers
void handleButtonClick() {
    Serial.println("Click: Switching Page...");
    if (currentPage == PAGE_WEATHER) {
        currentPage = PAGE_CALENDAR;
        displayCalendarPage(false); // Full refresh
    } else {
        currentPage = PAGE_WEATHER;
        displayWeatherDashboard(false); // Full refresh
    }
}

void handleButtonDoubleClick() {
    Serial.println("Double Click: Manual sleep triggered");
    enterDeepSleep();
}

void handleButtonLongPress() {
    Serial.println("Long Press: Refreshing Current Page...");
    if (currentPage == PAGE_WEATHER) {
        displayWeatherDashboard(false); // Full refresh
    } else {
        displayCalendarPage(false); // Full refresh
    }
}

unsigned long lastMqttRetry = 0;
int mqttRetryCounter = 0; // New counter for MQTT retries
bool wifiWarningShown = false;
bool mqttWarningShown = false;
bool mqttGiveUp = false; // Add give up flag

void setup() {
  Serial.begin(115200);          // ← 提前到最前面
  delay(100);                    // 等串口稳定
  printf("EPD_4IN2 WiFi Demo\r\n");
  DEV_Module_Init();             // 里面的 Serial.begin 重复调用无害
  
  // LED Status Indicator
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Turn on LED when awake

  // Bye Signal Pin
  pinMode(BYE_SIGNAL_PIN, OUTPUT);
  digitalWrite(BYE_SIGNAL_PIN, HIGH); // Default HIGH

  // Mode Pin Indicator (Pull-up)
  pinMode(MODE_PIN, INPUT_PULLUP);
  delay(100); // Wait for pin voltage to stabilize
  
  int modeState = digitalRead(MODE_PIN);
  Serial.printf("Mode Pin (GPIO %d) State: %d (%s)\n", MODE_PIN, modeState, (modeState == HIGH) ? "HIGH" : "LOW");
  
  // Load Config
  loadConfig();
  
  // Check wakeup reason for Battery Mode
  if (modeState == HIGH) {
      esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
      if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
          Serial.println("Wakeup: Timer (Battery Mode). Auto-refresh and sleep.");
          configMode = false;
      } else {
          // Button wakeup, power on, or serial wakeup
          Serial.println("Wakeup: User/Power-on (Battery Mode). Staying awake for Web UI.");
          configMode = true;
          wakeTime = millis();
      }
  } else {
      Serial.println("Mode: DC Powered. Staying awake.");
      configMode = true; // Stay awake in DC mode
  }
  
  // Check for low battery at startup
  float currentVoltage = getBatteryVoltage();
  if (modeState == HIGH && currentVoltage > 0.1 && currentVoltage < config.low_battery_threshold) {
      Serial.printf("Low Battery Detected: %.2fV (Threshold: %.2fV)\n", currentVoltage, config.low_battery_threshold);
      String warnMsg = "LOW BATTERY WARNING!\nVoltage: " + String(currentVoltage, 2) + "V\nPlease charge the device.";
      displayMessage(warnMsg);
      
      // Stop execution and go to deep sleep if possible or just loop forever
      // For now, we loop forever to prevent further battery drain from WiFi/Display
      while(1) {
          delay(1000);
      }
  }
  
  Serial.print("Air Quality Topic: ");
  Serial.println(config.mqtt_air_quality_topic);

  // Serial2 Init (Used for "bye" signal)
  Serial2.begin(9600, SERIAL_8N1, 16, 17);





  // Setup WiFi
  bool enableAP = false;
  
  if (strlen(config.wifi_ssid) > 0) {
      WiFi.mode(WIFI_STA);
      
      // Static IP Config
      if (config.use_static_ip) {
          IPAddress ip, gw, mask, dns;
          if (ip.fromString(config.static_ip) && gw.fromString(config.static_gw) && mask.fromString(config.static_mask)) {
               if (strlen(config.static_dns) > 0) dns.fromString(config.static_dns);
               else dns.fromString("8.8.8.8");
               
               if (!WiFi.config(ip, gw, mask, dns)) {
                   Serial.println("STA Failed to configure");
               } else {
                   Serial.println("Static IP Configured");
               }
          } else {
              Serial.println("Invalid Static IP Config");
          }
      }

      WiFi.begin(config.wifi_ssid, config.wifi_pass);
      
      int retry = 0;
      while (WiFi.status() != WL_CONNECTED && retry < 60) { // Increased to 30 seconds (was 15s)
          delay(500);
          retry++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
          Serial.println("WiFi Connected!");
          Serial.print("IP Address: ");
          Serial.println(WiFi.localIP());
          // AP remains disabled unless WiFi fails or is not configured
      } else {
          Serial.println("WiFi Timeout. Enabling AP.");
          enableAP = true;
          WiFi.mode(WIFI_AP);
      }
  } else {
      enableAP = true;
      WiFi.mode(WIFI_AP);
  }
  
  if (enableAP) {
      // Append MAC suffix to AP SSID using raw bytes for reliability
      uint8_t mac[6];
      WiFi.macAddress(mac);
      Serial.printf("Raw MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      
      char macSuffix[5];
      sprintf(macSuffix, "%02X%02X", mac[4], mac[5]);
      
      ap_ssid = String(DEFAULT_AP_SSID_BASE) + "-" + String(macSuffix);
      
      WiFi.softAP(ap_ssid.c_str());
      Serial.println("AP Started: " + ap_ssid);
      displayMessage("WiFi Timeout!\nAP Started: " + ap_ssid + "\nIP: 192.168.4.1");
  }
  
  // Setup NTP (Moved to inside MQTT connect block)
  /*
  if (strlen(config.ntp_server) > 0) {
      timeClient.setPoolServerName(config.ntp_server);
  } else {
      timeClient.setPoolServerName("ntp.aliyun.com");
  }
  timeClient.setTimeOffset(28800);
  timeClient.begin();
  */
  
  // Setup MQTT
  if (strlen(config.mqtt_server) > 0 && WiFi.status() == WL_CONNECTED) {
      client.setServer(config.mqtt_server, config.mqtt_port);
      client.setCallback(mqttCallback);
      client.setBufferSize(4096); // Increase buffer for large JSON
      
      // Attempt connection
      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);

      int mqttRetry = 0;
      bool mqttConnected = false;
      while (mqttRetry < 30 && !mqttConnected) { // 15 seconds
          if (client.connect(clientId.c_str(), config.mqtt_user, config.mqtt_pass)) {
              mqttConnected = true;
          } else {
              delay(500);
              mqttRetry++;
          }
      }
      
      if (mqttConnected) {
          Serial.println("MQTT Connected (Setup)");
          
          // Disable AP ONLY if it was enabled (optional, but safe)
          if (enableAP) {
              WiFi.softAPdisconnect(true);
              WiFi.mode(WIFI_STA);
              Serial.println("MQTT Connected! AP Disabled.");
          }
          
          // Setup NTP (Only after MQTT is connected)
          const char* ntp1 = (strlen(config.ntp_server) > 0) ? config.ntp_server : "ntp.aliyun.com";
          const char* ntp2 = (strlen(config.ntp_server_2) > 0) ? config.ntp_server_2 : "ntp.tencent.com";
          
          timeClient.setPoolServerName(ntp1);
          timeClient.setTimeOffset(28800);
          timeClient.begin(); // Start NTP
          
          Serial.printf("Waiting for NTP (%s)...", ntp1);
          // Reduced retry count since we are already connected to internet
          int retry = 0;
          bool ntpSynced = false;
          bool usingSecondary = false;
          
          while(retry < 10) { 
              if (timeClient.forceUpdate()) {
                  ntpSynced = true;
                  break;
              }
              delay(500);
              retry++;
              
              // Switch to secondary after 5 attempts
              if (retry == 5 && !usingSecondary && strlen(ntp2) > 0) {
                  Serial.printf("\nSwitching to Secondary NTP (%s)...", ntp2);
                  timeClient.setPoolServerName(ntp2);
                  usingSecondary = true;
              }
          }
          if (ntpSynced) {
              Serial.println(" Synced");
              Serial.println(timeClient.getFormattedTime());
          } else {
              Serial.println(" Timeout (Will retry in background)");
          }

          client.subscribe(config.mqtt_topic);
          client.subscribe(config.mqtt_weather_topic);
          client.subscribe(config.mqtt_date_topic);
          client.subscribe(config.mqtt_env_topic);
          client.subscribe(config.mqtt_calendar_topic);
          client.subscribe(config.mqtt_shift_topic);
          client.subscribe(config.mqtt_air_quality_topic);
          client.subscribe(config.mqtt_unified_topic); // Add unified topic subscription
          
          client.loop(); // Process incoming messages (e.g. SUBACK)
          delay(100);

          // Send Weather Request on Connect
          if (client.publish(config.mqtt_request_topic, "get")) {
              Serial.println("Weather Request sent (Setup)");
          } else {
              Serial.println("Weather Request failed (Setup)");
          }
      } else {
          Serial.print("MQTT Connect failed, rc=");
          Serial.println(client.state());
          displayMessage("MQTT Connect failed!\nCheck Broker Config.");
      }
  }



  server.on("/", handleRoot);
  server.on("/setPage", HTTP_POST, handleSetPage);
  server.on("/files", handleFileManager);
  server.on("/setText", handleSetText);
  server.on("/saveConfig", handleSaveConfig);
  server.on("/mqtt_config", handleMqttConfig);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/update", HTTP_GET, handleUpdate);
  server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      if (Update.hasError()) {
          server.send(500, "text/plain", "Update Failed");
      } else {
          server.send(200, "text/html", "Update Success. Rebooting...");
          delay(1000);
          ESP.restart();
      }
  }, handleUpdateFirmware);
  server.on("/upload", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleUpload);
  // handleDisplayImage route removed
  server.on("/delete", handleDelete);
  server.begin();
  
  printf("HTTP server started\r\n");

  // Setup OTA
  ArduinoOTA.setHostname("EPD-4in2-Demo");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    Serial.println("Start updating " + type);
    displayMessage("OTA Updating...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Setup Button
  button.attachClick(handleButtonClick);
  button.attachDoubleClick(handleButtonDoubleClick);
  button.attachLongPressStart(handleButtonLongPress);

  // Initial Display
// Initial Display - screen message suppressed
  if (WiFi.status() == WL_CONNECTED) {
      wifiWarningShown = false;
  } else {
      wifiWarningShown = true;
  }
}

void loop() {
  button.tick();
  server.handleClient();
  
  // WiFi Handling & Blocking
  if (strlen(config.wifi_ssid) > 0) {
      if (WiFi.status() != WL_CONNECTED) {
          if (!wifiWarningShown) {
              Serial.println("WiFi Lost, showing warning");
              displayMessage("WiFi Connection Lost\nAP Enabled: " + ap_ssid);
              wifiWarningShown = true;
              
              // Ensure AP is active
              if ((WiFi.getMode() & WIFI_AP) == 0) {
                   WiFi.mode(WIFI_AP_STA);
                   WiFi.softAP(ap_ssid.c_str());
              }
          }
          // Do NOT return here, allow server.handleClient() to run in loop
      } else {
          if (wifiWarningShown) {
              Serial.println("WiFi Restored");
              wifiWarningShown = false;
          }
      }
  }

  // MQTT Blocking Check (If failed to connect, stop everything else)
  if (mqttGiveUp) {
      if (!mqttWarningShown) {
           Serial.println("MQTT Failed (Persistent). AP Enabled.");
           // Ensure AP is active for config
           if ((WiFi.getMode() & WIFI_AP) == 0) {
                WiFi.mode(WIFI_AP_STA);
                WiFi.softAP(ap_ssid.c_str());
           }
           String msg = "MQTT Connect Failed\nConfig via AP: " + ap_ssid + "\nOr IP: " + WiFi.localIP().toString();
           displayMessage(msg);
           mqttWarningShown = true;
      }
      // Do NOT return here, allow server.handleClient() to run in loop
  }

  ArduinoOTA.handle();

  if (strlen(config.mqtt_server) > 0 && WiFi.status() == WL_CONNECTED) {
      if (!client.connected()) {
          if (!mqttGiveUp) {
              unsigned long now = millis();
              if (now - lastMqttRetry > 5000) {
                  lastMqttRetry = now;
                  
                  if (reconnect()) {
                       mqttRetryCounter = 0;
                       mqttWarningShown = false;
                       // Disable AP if reconnected
                       WiFi.softAPdisconnect(true);
                       WiFi.mode(WIFI_STA);
                  } else {
                       mqttRetryCounter++;
                       Serial.printf("MQTT Reconnect failed (%d/3)\n", mqttRetryCounter);
                       
                       if (mqttRetryCounter >= 3 && !mqttWarningShown) {
                            displayMessage("MQTT Connection Failed\nBroker not reachable.");
                            mqttWarningShown = true;
                            // Optionally set mqttGiveUp = true if you want to stop retrying
                            // mqttGiveUp = true; 
                       }
                  }
              }
          }
      } else {
          client.loop();
          mqttRetryCounter = 0; // Reset counter on success
          mqttGiveUp = false; // Reset give up flag on successful connection
          mqttWarningShown = false;
      }
  }
  
  // Handle deferred updates
  if (millis() - lastUpdateTrigger > UPDATE_DELAY_MS) {
      if (updateWeatherPending || updateEnvPending || updateDatePending || (updateCalendarPending && currentPage == PAGE_WEATHER)) {
          if (currentPage == PAGE_WEATHER) {
               Serial.printf("Triggering Deferred Weather Update (Full: %s)\n", fullRefreshPending ? "Yes" : "No");
               displayWeatherDashboard(!fullRefreshPending, updateWeatherPending); // Send signal ONLY if weather MQTT triggered this
               refreshDone = true; // Mark as refreshed for battery mode sleep
               lastRefreshTime = millis(); // Set timestamp for sleep delay
          }
          updateWeatherPending = false;
          updateEnvPending = false;
          updateDatePending = false;
          fullRefreshPending = false; // Reset full refresh flag
          if (currentPage == PAGE_WEATHER) updateCalendarPending = false; // Only clear if we actually refreshed
      }
      
      if (updateCalendarPending) {
          if (currentPage == PAGE_CALENDAR) {
               Serial.println("Triggering Deferred Calendar Update");
               displayCalendarPage(true);
               refreshDone = true; // Mark as refreshed for battery mode sleep
               lastRefreshTime = millis(); // Set timestamp for sleep delay
          }
          updateCalendarPending = false;
      }
  }
  
  // Handle Immediate UI Page Switch
  if (switchPagePending) {
      Serial.println("UI Page Switch Triggered");
      if (currentPage == PAGE_WEATHER) {
          displayWeatherDashboard(false); // Full refresh
      } else if (currentPage == PAGE_CALENDAR) {
          displayCalendarPage(false); // Full refresh
      }
      switchPagePending = false;
  }
  
  // Optimized NTP Sync Strategy (Only if MQTT is connected)
  static unsigned long lastNtpRetry = 0;
  static int ntpRetryCount = 0;
  static bool usingSecondaryNtp = false;

  if (client.connected()) {
      if (!timeClient.isTimeSet()) {
          if (millis() - lastNtpRetry > 5000) { // Retry every 5 seconds
              lastNtpRetry = millis();
              Serial.println("NTP not synced, forcing update...");
              if (timeClient.forceUpdate()) {
                   Serial.println("NTP Synced (Loop)");
                   ntpRetryCount = 0;
              } else {
                   ntpRetryCount++;
                   if (ntpRetryCount >= 3) {
                       ntpRetryCount = 0;
                       usingSecondaryNtp = !usingSecondaryNtp;
                       
                       const char* ntp1 = (strlen(config.ntp_server) > 0) ? config.ntp_server : "ntp.aliyun.com";
                       const char* ntp2 = (strlen(config.ntp_server_2) > 0) ? config.ntp_server_2 : "ntp.tencent.com";
                       const char* nextServer = usingSecondaryNtp ? ntp2 : ntp1;
                       
                       Serial.printf("NTP Failed 3 times. Switching to: %s\n", nextServer);
                       timeClient.setPoolServerName(nextServer);
                   }
              }
          }
      } else {
          // In DATE mode, we don't need frequent NTP updates as we don't show seconds/minutes
          if (config.ui_mode == 0) {
              timeClient.update();
          }
      }
  }
  
  // Custom Full Refresh Timer
  static unsigned long lastFullRefreshTime = millis();
  if (config.full_refresh_period > 0) {
      if (millis() - lastFullRefreshTime > (unsigned long)config.full_refresh_period * 60 * 1000) {
          lastFullRefreshTime = millis();
          Serial.println("Custom Full Refresh Triggered");
          if (currentPage == PAGE_WEATHER) {
               displayWeatherDashboard(false); // Full refresh
          } else if (currentPage == PAGE_CALENDAR) {
               displayCalendarPage(false); // Full refresh
          }
      }
  }

  // Periodic Request Timer
   static unsigned long lastRequestTime = millis();
   if (config.request_interval > 0 && client.connected()) {
       if (millis() - lastRequestTime > (unsigned long)config.request_interval * 60 * 1000) { // Convert minutes to ms
           lastRequestTime = millis();
           Serial.println("Sending Periodic Weather Request");
           client.publish(config.mqtt_request_topic, "get");
       }
   }
  
  // Check for minute change
  int currentMinute = timeClient.getMinutes();
  if (lastMinute != -1 && currentMinute != lastMinute) {
      // Minute changed
      int currentHour = timeClient.getHours();
      
      // If it is midnight (00:00), refresh both pages fully to update date
      if (currentHour == 0 && currentMinute == 0) {
          Serial.println("Midnight! Refreshing page...");
          if (currentPage == PAGE_WEATHER) {
               displayWeatherDashboard(false); // Full update
          } else if (currentPage == PAGE_CALENDAR) {
               displayCalendarPage(false); // Full update
          }
      } else {
          // Normal minute update (only for weather page in TIME mode)
          if (config.ui_mode == 0 && currentPage == PAGE_WEATHER && currentForecastCount > 0) {
              Serial.println("Minute changed, updating display (Partial)...");
              displayWeatherDashboard(true);
          }
      }
  }
  lastMinute = currentMinute;

  // Battery Mode Deep Sleep Logic
  if (digitalRead(MODE_PIN) == HIGH) {
      // 1. If we are in Config Mode (stay awake for user defined mins)
      if (configMode) {
          if (millis() - wakeTime > (unsigned long)config.config_timeout * 60000) {
              Serial.println("Config mode timeout. Entering sleep...");
              enterDeepSleep();
          }
      } 
      // 2. If we are NOT in config mode, sleep according to config after refresh is done
      else {
          // If at least one refresh has occurred and it's been the configured delay since last refresh
          if (refreshDone && (millis() - lastRefreshTime > (unsigned long)config.sleep_delay * 1000)) {
              Serial.println("Update cycle complete. Entering sleep...");
              enterDeepSleep();
          }
          
          // Failsafe: if we've been awake for more than 2 minutes without refresh, sleep anyway to save battery
          if (millis() > 120000) { 
              Serial.println("Failsafe: Awake too long without refresh. Sleeping...");
              enterDeepSleep();
          }
      }
  }
}

void enterDeepSleep() {
    Serial.println("Entering Deep Sleep Mode (Extreme Savings)...");
    
    // 0. Disable remaining peripherals to prevent leakage
    pinMode(LED_PIN, INPUT);
    pinMode(MODE_PIN, INPUT);
    pinMode(BYE_SIGNAL_PIN, INPUT);
    if (config.adc_pin >= 0) pinMode(config.adc_pin, INPUT);
    
    // 1. Shut down Radio and Wireless
    if (client.connected()) client.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop(); // Stop Bluetooth explicitly
    
    // 2. Put EPD to deep sleep and isolate its pins
    hibernateEPD();
    
    // 3. Configure Wakeup Sources
    // Ext0 Wakeup: BUTTON_PIN (GPIO 0), Active LOW
    // The ESP32 RTC domain will handle the pull-up internally if configured
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0); 
    
    // Timer Wakeup: From Config
    if (config.request_interval > 0) {
        uint64_t sleepTime = (uint64_t)config.request_interval * 60 * 1000000ULL;
        esp_sleep_enable_timer_wakeup(sleepTime);
        Serial.printf("Timer wakeup scheduled in %d minutes\n", config.request_interval);
    }
    
    // 4. Final power domain optimizations
    // Optional: Turn off more power domains if not using RTC memory
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

    // 5. Start Sleep
    Serial.flush();
    delay(100);
    esp_deep_sleep_start();
}
