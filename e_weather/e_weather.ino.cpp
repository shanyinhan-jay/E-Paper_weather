# 1 "C:\\Users\\Administrator\\AppData\\Local\\Temp\\tmpai6f3rlt"
#include <Arduino.h>
# 1 "D:/ink/E-Paper_weather/e_weather/e_weather.ino"
# 19 "D:/ink/E-Paper_weather/e_weather/e_weather.ino"
#include "DEV_Config.h"
#include "Local_EPD_4in2.h"
#include "GUI_Paint.h"

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include <stdlib.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "icon/icon_m.h"
#include "icon/icon_s.h"
#include "icon/icon_o.h"
#include "icon/icon_w.h"
#include "AppConfig.h"
#include "WebHandler.h"

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <OneButton.h>
#include <vector>

#ifndef ENABLE_DRIVER_LOCAL
#define ENABLE_DRIVER_LOCAL 1
#endif
#ifndef ENABLE_DRIVER_GX2IC
#define ENABLE_DRIVER_GX2IC 0
#endif

#if ENABLE_DRIVER_GX2IC
#include <GxEPD2_2IC_BW.h>
#endif

extern const uint8_t u8g2_font_wqy12_t_gb2312[] U8X8_PROGMEM;


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
OneButton button(BUTTON_PIN, true);

const char* DEFAULT_AP_SSID_BASE = "EPD-Display";
String ap_ssid = DEFAULT_AP_SSID_BASE;

#define MAX_SHIFT_EVENTS 100

Config config;

enum DisplayDriverType : uint8_t {
    DISPLAY_DRIVER_LOCAL = 0,
    DISPLAY_DRIVER_GX2IC = 1
};

#if ENABLE_DRIVER_GX2IC

GxEPD2_2IC_BW<GxEPD2_2IC_420_A03, GxEPD2_2IC_420_A03::HEIGHT> gxDisplay(
    GxEPD2_2IC_420_A03(EPD_CS_PIN, EPD_SCK_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN)
);
#endif

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


bool updateWeatherPending = false;
bool updateEnvPending = false;
bool updateDatePending = false;
bool fullRefreshPending = false;
unsigned long lastUpdateTrigger = 0;
const unsigned long UPDATE_DELAY_MS = 200;


bool refreshDone = false;
unsigned long lastRefreshTime = 0;
bool configMode = false;
unsigned long wakeTime = 0;


void enterDeepSleep();
static void setAdcMeasurementPower(bool enabled);
static bool isDriverAvailable(int driver);
static int getDefaultAvailableDriver();
static int getActiveDisplayDriver();
static void applyDisplayDriverConfigFallback();
static void epdFlushFrame(bool partial_update, UBYTE* image);
void loadConfig();
void saveConfig();
float getBatteryVoltage();
void publishBatteryVoltage();
void displayMessageWithBitmap(String text, const unsigned char* bitmap, int bitmapWidth, int bitmapHeight);
void displayMessage(String text);
char getIconChar(String cond);
void drawNoIconPlaceholder(int x, int y, int w, int h);
static String normalizeWeatherIconCode(const String& rawCode);
void displayWeatherDashboard(bool partial_update, bool sendSignal);
void hibernateEPD();
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool reconnect();
void handleButtonClick();
void handleButtonDoubleClick();
void handleButtonLongPress();
void setup();
void loop();
#line 151 "D:/ink/E-Paper_weather/e_weather/e_weather.ino"
static void setAdcMeasurementPower(bool enabled) {
    digitalWrite(ADC_SWITCH_EN_PIN, enabled ? HIGH : LOW);
}

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

        targetColor = (color == 1) ? WHITE : BLACK;
    } else {

        targetColor = (color == 1) ? BLACK : WHITE;
    }

    if (scale == 1.0) {
        if (x < 0 || x >= EPD_4IN2_WIDTH || y < 0 || y >= EPD_4IN2_HEIGHT) return;
        Paint_SetPixel(x, y, targetColor);
    } else {

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

static inline bool jsonHasKey(const JsonVariantConst& obj, const char* key) {
  return !obj[key].isNull();
}

static bool isDriverAvailable(int driver) {
    if (driver == DISPLAY_DRIVER_LOCAL) {
#if ENABLE_DRIVER_LOCAL
        return true;
#else
        return false;
#endif
    }
    if (driver == DISPLAY_DRIVER_GX2IC) {
#if ENABLE_DRIVER_GX2IC
        return true;
#else
        return false;
#endif
    }
    return false;
}

static int getDefaultAvailableDriver() {
#if ENABLE_DRIVER_LOCAL
    return DISPLAY_DRIVER_LOCAL;
#elif ENABLE_DRIVER_GX2IC
    return DISPLAY_DRIVER_GX2IC;
#else
    return DISPLAY_DRIVER_LOCAL;
#endif
}

static int getActiveDisplayDriver() {
    if (isDriverAvailable(config.display_driver)) return config.display_driver;
    return getDefaultAvailableDriver();
}

static void applyDisplayDriverConfigFallback() {
    if (!isDriverAvailable(config.display_driver)) {
        int fallback = getDefaultAvailableDriver();
        Serial.printf("Configured display_driver=%d unavailable, fallback=%d\n", config.display_driver, fallback);
        config.display_driver = fallback;
    }
}

static void epdFlushFrame(bool partial_update, UBYTE* image) {
    int driver = getActiveDisplayDriver();
    if (driver == DISPLAY_DRIVER_LOCAL) {
#if ENABLE_DRIVER_LOCAL
        if (partial_update) {
            Local_EPD_4IN2_Init_Partial();
            Local_EPD_4IN2_PartialDisplay(0, 0, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, image);
        } else {
            Local_EPD_4IN2_Init();
            Local_EPD_4IN2_Display(image);
        }
#endif
        return;
    }

#if ENABLE_DRIVER_GX2IC

    gxDisplay.init(0);
    gxDisplay.setRotation(0);
    gxDisplay.drawImage(image, 0, 0, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, false, false, false);
#endif
}

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
          strlcpy(config.device_name, doc["device_name"] | "EPD-Display", sizeof(config.device_name));
          strlcpy(config.mqtt_server, doc["mqtt_server"] | "", sizeof(config.mqtt_server));
          config.mqtt_port = doc["mqtt_port"] | 1883;
          strlcpy(config.mqtt_user, doc["mqtt_user"] | "", sizeof(config.mqtt_user));
          strlcpy(config.mqtt_pass, doc["mqtt_pass"] | "", sizeof(config.mqtt_pass));
          strlcpy(config.mqtt_topic, doc["mqtt_topic"] | "epd/text", sizeof(config.mqtt_topic));
          strlcpy(config.mqtt_weather_topic, doc["mqtt_weather_topic"] | "epd/weather", sizeof(config.mqtt_weather_topic));
          strlcpy(config.mqtt_date_topic, doc["mqtt_date_topic"] | "epd/date", sizeof(config.mqtt_date_topic));
          strlcpy(config.mqtt_env_topic, doc["mqtt_env_topic"] | "epd/env", sizeof(config.mqtt_env_topic));
          strlcpy(config.mqtt_shift_topic, doc["mqtt_shift_topic"] | "epd/shift", sizeof(config.mqtt_shift_topic));
          strlcpy(config.mqtt_air_quality_topic, doc["mqtt_air_quality_topic"] | "epd/air_quality", sizeof(config.mqtt_air_quality_topic));
          strlcpy(config.mqtt_battery_topic, doc["mqtt_battery_topic"] | "shanyinhan/epd/battery", sizeof(config.mqtt_battery_topic));
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
          config.display_driver = doc["display_driver"] | 0;
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

  applyDisplayDriverConfigFallback();


  if (BlackImage == NULL) {
      UWORD Imagesize = ((EPD_4IN2_WIDTH % 8 == 0) ? (EPD_4IN2_WIDTH / 8 ) : (EPD_4IN2_WIDTH / 8 + 1)) * EPD_4IN2_HEIGHT;
      BlackImage = (UBYTE *)malloc(Imagesize);
      if (BlackImage == NULL) {
          Serial.println("FATAL: BlackImage Malloc Failed in Setup!");
      } else {
          Serial.printf("BlackImage Allocated: %u bytes\n", Imagesize);

          memset(BlackImage, 0xFF, Imagesize);
      }
  }
}

void saveConfig() {
  JsonDocument doc;
  doc["wifi_ssid"] = config.wifi_ssid;
  doc["wifi_pass"] = config.wifi_pass;
  doc["device_name"] = config.device_name;
  doc["mqtt_server"] = config.mqtt_server;
  doc["mqtt_port"] = config.mqtt_port;
  doc["mqtt_user"] = config.mqtt_user;
  doc["mqtt_pass"] = config.mqtt_pass;
  doc["mqtt_topic"] = config.mqtt_topic;
  doc["mqtt_weather_topic"] = config.mqtt_weather_topic;
  doc["mqtt_date_topic"] = config.mqtt_date_topic;
  doc["mqtt_env_topic"] = config.mqtt_env_topic;
  doc["mqtt_shift_topic"] = config.mqtt_shift_topic;
  doc["mqtt_air_quality_topic"] = config.mqtt_air_quality_topic;
  doc["mqtt_battery_topic"] = config.mqtt_battery_topic;
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
  doc["display_driver"] = config.display_driver;
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
    setAdcMeasurementPower(true);
    delay(20);


    uint32_t sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(config.adc_pin);
        delay(10);
    }
    float avgAdc = sum / 10.0;



    float voltage = avgAdc * (3.3 / 4095.0) * config.adc_ratio;
    setAdcMeasurementPower(false);
    return voltage;
}

void publishBatteryVoltage() {
    Serial.printf("Attempting to publish battery voltage to: %s\n", config.mqtt_battery_topic);
    if (strlen(config.mqtt_battery_topic) > 0) {
        if (client.connected()) {
            float voltage = getBatteryVoltage();
            Serial.printf("Measured voltage: %.2fV\n", voltage);
            if (voltage > 0.1) {
                char payload[10];
                dtostrf(voltage, 4, 2, payload);
                if (client.publish(config.mqtt_battery_topic, payload, true)) {
                    Serial.printf("Battery voltage published: %sV to %s\n", payload, config.mqtt_battery_topic);
                } else {
                    Serial.println("Failed to publish battery voltage (MQTT Error)");
                }
            } else {
                Serial.println("Voltage too low to publish");
            }
        } else {
            Serial.println("Cannot publish: MQTT Client NOT connected");
        }
    } else {
        Serial.println("Cannot publish: mqtt_battery_topic is empty");
    }
}

void displayMessageWithBitmap(String text, const unsigned char* bitmap, int bitmapWidth, int bitmapHeight) {
    Serial.println("displayMessage start");
    Serial.printf("Free Heap: %u\n", ESP.getFreeHeap());

    DEV_Module_Init();

    if (BlackImage == NULL) {
         Serial.println("Error: BlackImage is NULL (Should be allocated in setup)");

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


        float scale = 1.0;
        paint_gfx.setScale(scale);
        int logicalWidth = EPD_4IN2_WIDTH / scale;
        int logicalHeight = EPD_4IN2_HEIGHT / scale;



        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2.setFontMode(1);
        u8g2.setForegroundColor(1);
        u8g2.setBackgroundColor(0);

        Serial.println("Processing Text");

        std::vector<String> lines;
        std::vector<String> bitmapCaptionLines;
        String currentLine = "";
        int len = text.length();
        int i = 0;

        while (i < len) {


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

                break;
            }

            String nextChar = text.substring(i, i + charLen);


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

        if (bitmap != nullptr && bitmapWidth > 0 && bitmapHeight > 0) {
            String bitmapCaption = "Scan the QR code for assistance.";
            String captionLine = "";
            int captionLen = bitmapCaption.length();
            int captionIndex = 0;

            while (captionIndex < captionLen) {
                int charLen = 1;
                unsigned char c = bitmapCaption[captionIndex];
                if (c >= 0xF0) charLen = 4;
                else if (c >= 0xE0) charLen = 3;
                else if (c >= 0xC0) charLen = 2;

                if (captionIndex + charLen > captionLen) {
                    break;
                }

                String nextChar = bitmapCaption.substring(captionIndex, captionIndex + charLen);
                if (u8g2.getUTF8Width((captionLine + nextChar).c_str()) > (logicalWidth - 10)) {
                    bitmapCaptionLines.push_back(captionLine);
                    captionLine = nextChar;
                } else {
                    captionLine += nextChar;
                }
                captionIndex += charLen;
            }

            if (captionLine.length() > 0) {
                bitmapCaptionLines.push_back(captionLine);
            }
        }

        Serial.println("Calculating Layout");

        int lineHeight = 20;
        int textHeight = lines.size() * lineHeight;
        int captionHeight = bitmapCaptionLines.size() * lineHeight;
        int captionGap = captionHeight > 0 ? 10 : 0;
        int imageGap = (bitmap != nullptr && bitmapWidth > 0 && bitmapHeight > 0) ? 12 : 0;
        int totalHeight = textHeight + captionGap + captionHeight + imageGap + bitmapHeight;

        int startY = (logicalHeight - totalHeight) / 2 + 16;
        if (bitmap != nullptr && bitmapWidth > 0 && bitmapHeight > 0) {
            startY -= 18;
        }
        if (startY < 20) startY = 20;


        Serial.println("Drawing Lines");
        for (const String& line : lines) {
            int w = u8g2.getUTF8Width(line.c_str());
            int x = (logicalWidth - w) / 2;
            if (x < 0) x = 0;

            u8g2.drawUTF8(x, startY, line.c_str());

            startY += lineHeight;
        }

        if (!bitmapCaptionLines.empty()) {
            startY += captionGap;
            for (const String& line : bitmapCaptionLines) {
                int w = u8g2.getUTF8Width(line.c_str());
                int x = (logicalWidth - w) / 2;
                if (x < 0) x = 0;

                u8g2.drawUTF8(x, startY, line.c_str());
                startY += lineHeight;
            }
        }

        if (bitmap != nullptr && bitmapWidth > 0 && bitmapHeight > 0) {
            int imageX = (logicalWidth - bitmapWidth) / 2;
            if (imageX < 0) imageX = 0;
            int imageY = startY + imageGap;
            paint_gfx.drawBitmap(imageX, imageY, bitmap, bitmapWidth, bitmapHeight, 1);
        }


        paint_gfx.setScale(1.0);

        Serial.println("Displaying");
        epdFlushFrame(false, BlackImage);
        hibernateEPD();


        Serial.println("displayMessage done");
    }
}

void displayMessage(String text) {
    displayMessageWithBitmap(text, nullptr, 0, 0);
}

char getIconChar(String cond) {
    char iconChar = 64;
    String c = cond;
    c.toLowerCase();


    if (c.indexOf("多云") >= 0 || c.indexOf("阴") >= 0) iconChar = 65;
    else if (c.indexOf("雨") >= 0) iconChar = 67;
    else if (c.indexOf("雪") >= 0) iconChar = 67;
    else if (c.indexOf("雷") >= 0) iconChar = 69;
    else if (c.indexOf("晴") >= 0) iconChar = 64;


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
# 781 "D:/ink/E-Paper_weather/e_weather/e_weather.ino"
    bool downscale = false;
    if (width > 60) {
        if (scale == 2) {
            scale = 1;
        } else if (scale == 1) {


             if (y > 60) {
                 downscale = true;
             }
        }
    }

    bmpFile.seek(dataOffset);


    uint32_t rowSize = (width * (depth / 8) + 3) & ~3;
    uint8_t *rowBuff = new (std::nothrow) uint8_t[rowSize];
    if (!rowBuff) {
        Serial.println("Memory allocation failed for rowBuff");
        bmpFile.close();
        return;
    }


    for (int row = 0; row < height; row++) {
        bmpFile.read(rowBuff, rowSize);

        if (downscale && (row % 2 != 0)) continue;


        int ty;
        if (downscale) {
             ty = y + (height - 1 - row) / 2;
        } else {
             ty = y + (height - 1 - row) * scale;
        }

        for (int col = 0; col < width; col++) {
             if (downscale && (col % 2 != 0)) continue;

             int b, g, r;
             int pxOffset = col * (depth / 8);

             b = rowBuff[pxOffset];
             g = rowBuff[pxOffset+1];
             r = rowBuff[pxOffset+2];


             int brightness = (r + g + b) / 3;
             uint16_t color = (brightness < 128) ? 1 : 0;


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


extern const uint8_t u8g2_font_logisoso50_tf[] U8G2_FONT_SECTION("u8g2_font_logisoso50_tf");

enum class IconRenderMode : uint8_t {
    Exact = 0,
    Scale2x = 1,
    Downscale2x = 2,
};

void drawIconFromProgmem(const unsigned char* data, int x, int y, int w, int h, IconRenderMode mode = IconRenderMode::Exact) {
    if (!data) return;



    int bytesPerRow = (w + 7) / 8;

    for (int row = 0; row < h; row++) {
        if (mode == IconRenderMode::Downscale2x && (row % 2 != 0)) continue;

        for (int col = 0; col < w; col++) {
            if (mode == IconRenderMode::Downscale2x && (col % 2 != 0)) continue;


            int byteIdx = row * bytesPerRow + (col / 8);
            int bitIdx = 7 - (col % 8);

            uint8_t b = pgm_read_byte(&data[byteIdx]);

            if (b & (1 << bitIdx)) {
                if (mode == IconRenderMode::Downscale2x) {
                    paint_gfx.drawPixel(x + col / 2, y + row / 2, 1);
                } else if (mode == IconRenderMode::Scale2x) {

                    paint_gfx.drawPixel(x + col * 2, y + row * 2, 1);
                    paint_gfx.drawPixel(x + col * 2 + 1, y + row * 2, 1);
                    paint_gfx.drawPixel(x + col * 2, y + row * 2 + 1, 1);
                    paint_gfx.drawPixel(x + col * 2 + 1, y + row * 2 + 1, 1);
                } else {
                    paint_gfx.drawPixel(x + col, y + row, 1);
                }
            }
        }
    }
}

void drawNoIconPlaceholder(int x, int y, int w, int h) {
    paint_gfx.drawRect(x, y, w, h, 1);
    u8g2.setFont(u8g2_font_5x7_tf);
    const char* label = "no icon";
    int textW = u8g2.getUTF8Width(label);
    int textX = x + (w - textW) / 2;
    if (textX < x + 2) textX = x + 2;
    int textY = y + (h / 2) + 3;
    u8g2.drawUTF8(textX, textY, label);
}

static String normalizeWeatherIconCode(const String& rawCode) {
    String code = rawCode;
    code.trim();
    if (code.length() > 5) {
        String lower = code;
        lower.toLowerCase();
        if (lower.endsWith("-fill")) {
            code = code.substring(0, code.length() - 5);
        }
    }
    return code;
}

void displayWeatherDashboard(bool partial_update, bool sendSignal) {
    DEV_Module_Init();

    if (BlackImage == NULL) {
         Serial.println("Error: BlackImage is NULL (Should be allocated in setup)");

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



        paint_gfx.drawFastVLine(200, 5, 135, 1);


        drawIconFromProgmem(getOtherIconData("tem"), 375, 5, 20, 36);


        if (config.ui_mode == 0) {

            u8g2.setFont(u8g2_font_logisoso50_tf);
            String timeStr = timeClient.getFormattedTime().substring(0, 5);
            int tWidth = u8g2.getUTF8Width(timeStr.c_str());
            int timeX = 100 - (tWidth / 2);
            u8g2.drawUTF8(timeX, 65, timeStr.c_str());

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


            time_t now = timeClient.getEpochTime();
            struct tm *t_now = gmtime(&now);

            int boxW = 150;
            int boxH = 68;
            int boxX = 100 - (boxW / 2);
            int boxY = 5;
            int cornerR = 10;



            paint_gfx.fillRoundRect(boxX, boxY, boxW, boxH, cornerR, 0);


            paint_gfx.fillRoundRect(boxX, boxY, boxW / 2, boxH, cornerR, 1);
            paint_gfx.fillRect(boxX + (boxW / 2) - cornerR, boxY, cornerR, boxH, 1);


            paint_gfx.drawRoundRect(boxX, boxY, boxW, boxH, cornerR, 1);


            paint_gfx.drawFastVLine(boxX + (boxW / 2), boxY, boxH, 1);


            u8g2.setFont(u8g2_font_logisoso50_tf);
            int ascent = u8g2.getFontAscent();
            int textY = boxY + (boxH + ascent) / 2;


            char monthStr[5];
            sprintf(monthStr, "%02d", t_now->tm_mon + 1);
            int mWidth = u8g2.getUTF8Width(monthStr);
            u8g2.setForegroundColor(0);
            u8g2.setBackgroundColor(1);
            u8g2.drawUTF8(boxX + (boxW / 4) - (mWidth / 2), textY, monthStr);


            char dayStr[5];
            sprintf(dayStr, "%02d", t_now->tm_mday);
            int dWidth = u8g2.getUTF8Width(dayStr);
            u8g2.setForegroundColor(1);
            u8g2.setBackgroundColor(0);
            u8g2.drawUTF8(boxX + (3 * boxW / 4) - (dWidth / 2), textY, dayStr);


            u8g2.setForegroundColor(1);
            u8g2.setBackgroundColor(0);



            if (weekDay.length() > 0) {

                const int dateCenterX = 100;
                const int weekIconW = 150;
                const int weekIconH = 27;
                const int weekIconX = dateCenterX - (weekIconW / 2);
                const int weekIconY = 75;
                String weekCode = "";
                if (weekDay.indexOf("一") >= 0 || weekDay.indexOf("Mon") >= 0) weekCode = "mon";
                else if (weekDay.indexOf("二") >= 0 || weekDay.indexOf("Tue") >= 0) weekCode = "tue";
                else if (weekDay.indexOf("三") >= 0 || weekDay.indexOf("Wed") >= 0) weekCode = "wed";
                else if (weekDay.indexOf("四") >= 0 || weekDay.indexOf("Thu") >= 0) weekCode = "thu";
                else if (weekDay.indexOf("五") >= 0 || weekDay.indexOf("Fri") >= 0) weekCode = "fri";
                else if (weekDay.indexOf("六") >= 0 || weekDay.indexOf("Sat") >= 0) weekCode = "sat";
                else if (weekDay.indexOf("日") >= 0 || weekDay.indexOf("Sun") >= 0) weekCode = "sun";
                const unsigned char* weekIcon = weekCode.length() > 0 ? getWeekIconData(weekCode) : NULL;

                if (weekIcon) {

                    drawIconFromProgmem(weekIcon, weekIconX, weekIconY, weekIconW, weekIconH, IconRenderMode::Exact);
                } else {

                    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
                    int wWidth = u8g2.getUTF8Width(weekDay.c_str());
                    u8g2.drawUTF8(100 - (wWidth / 2), 85, weekDay.c_str());
                }
            }

            u8g2.setFont(u8g2_font_wqy12_t_gb2312);

            if (lunarDate.length() > 0) {
                int ldWidth = u8g2.getUTF8Width(lunarDate.c_str());
                u8g2.drawUTF8(100 - (ldWidth / 2), 115, lunarDate.c_str());
            }

            if (termInfo.length() > 0) {
                int tiWidth = u8g2.getUTF8Width(termInfo.c_str());
                u8g2.drawUTF8(100 - (tiWidth / 2), 135, termInfo.c_str());
            }
        }


        if (currentForecastCount > 0) {
            int iconX = 210;
            int textCenterX = 340;
            int panelCenterX = 300;

            int currentHour = timeClient.getHours();
            bool isNight = (currentHour >= config.day_end_hour || currentHour < config.day_start_hour);
            String iconCode = isNight ? currentForecast[0].icon_night : currentForecast[0].icon_day;
            String condText = isNight ? currentForecast[0].cond_night : currentForecast[0].cond_day;
            if (iconCode.length() == 0) iconCode = currentForecast[0].icon_day;
            if (condText.length() == 0) condText = currentForecast[0].cond_day;
            String lookupIconCode = normalizeWeatherIconCode(iconCode);


            bool iconDrawn = false;
            if (lookupIconCode.length() > 0) {

                const unsigned char* iconData = getMediumIconData(lookupIconCode);
                if (iconData) {
                    drawIconFromProgmem(iconData, iconX - 4, 25, 72, 72, IconRenderMode::Exact);
                    iconDrawn = true;
                }
            }


            if (!iconDrawn && lookupIconCode.length() > 0) {
                String iconPath = "/icons/" + lookupIconCode + ".bmp";
                if (LittleFS.exists(iconPath)) {
                    drawBmp(iconPath, iconX, 25, 1);
                    iconDrawn = true;
                }
            }

            if (!iconDrawn) {
                drawNoIconPlaceholder(iconX, 25, 72, 72);
            }


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


            String windDirPart = "";
            if (currentForecast[0].wind_dir.length() > 0) windDirPart = currentForecast[0].wind_dir;

            String windScPart = "";
            String jiPart = "";
            if (currentForecast[0].wind_sc.length() > 0) {
                windScPart = currentForecast[0].wind_sc;
                jiPart = "级";
            }


            String weatherDetailPart1 = condText + "  " + windDirPart + windScPart;

            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            int wdW1 = u8g2.getUTF8Width(weatherDetailPart1.c_str());
            int wdW2 = (jiPart.length() > 0) ? u8g2.getUTF8Width(jiPart.c_str()) : 0;


            String aqPart = "";
            if (airCategory.length() > 0) {
                 aqPart = "  " + airCategory;
            }
            int wdW3 = (aqPart.length() > 0) ? u8g2.getUTF8Width(aqPart.c_str()) : 0;


            int gap = (jiPart.length() > 0) ? 3 : 0;
            int totalW = wdW1 + gap + wdW2 + wdW3;

            int wdX = panelCenterX - (totalW / 2);
            int wdY = 115;


            u8g2.drawUTF8(wdX, wdY, weatherDetailPart1.c_str());


            if (jiPart.length() > 0) {
                u8g2.drawUTF8(wdX + wdW1 + gap, wdY, jiPart.c_str());
                u8g2.drawUTF8(wdX + wdW1 + gap + 1, wdY, jiPart.c_str());
            }


            if (aqPart.length() > 0) {
                u8g2.drawUTF8(wdX + wdW1 + gap + wdW2, wdY, aqPart.c_str());
            }


            if (config.ui_mode == 0) {
                if (indoorTemp.length() > 0) {
                     u8g2.setFont(u8g2_font_wqy12_t_gb2312);


                     String envPart1 = "T:" + indoorTemp + "°C  H:" + indoorHumi;
                     String envPart2 = "%";

                     int inW1 = u8g2.getUTF8Width(envPart1.c_str());
                     int inW2 = u8g2.getUTF8Width(envPart2.c_str());
                     int inGap = 3;

                     int totalInW = inW1 + inGap + inW2;
                     int inX = panelCenterX - (totalInW / 2);
                     int inY = 135;


                     u8g2.drawUTF8(inX, inY, envPart1.c_str());
                     u8g2.drawUTF8(inX + inW1 + inGap, inY, envPart2.c_str());
                }
            } else {

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


        paint_gfx.drawFastHLine(0, 155, EPD_4IN2_WIDTH, 2);


        int startY = 155;


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
            if (highs[i] < minTemp) minTemp = highs[i];
            if (lows[i] > maxTemp) maxTemp = lows[i];
            count++;
        }


        if (maxTemp == minTemp) { maxTemp++; minTemp--; }
        int tempRange = maxTemp - minTemp;
        if (tempRange == 0) tempRange = 1;


        int chartTop = startY + 60;
        int chartBottom = startY + 90;
        int chartHeight = chartBottom - chartTop;

        for(int i=1; i<currentForecastCount && i<=6; i++) {
            int x1 = (i - 1) * EPD_4IN2_WIDTH / 6;
            int x2 = i * EPD_4IN2_WIDTH / 6;
            int centerX = x1 + ((x2 - x1) / 2);
            xCoords[i] = centerX;


            String dateShort = currentForecast[i].date;
            if (dateShort.length() >= 10) dateShort = dateShort.substring(5);
            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            int dWidth = u8g2.getUTF8Width(dateShort.c_str());
            u8g2.drawUTF8(centerX - (dWidth/2), startY , dateShort.c_str());




            int iconY = startY + 16;
            String dayLookupCode = normalizeWeatherIconCode(currentForecast[i].icon_day);

            bool dayIconDrawn = false;
            if (dayLookupCode.length() > 0) {

                const unsigned char* iconData = getSmallIconData(dayLookupCode);
                if (iconData) {
                    drawIconFromProgmem(iconData, centerX - 18, iconY - 7, 36, 36, IconRenderMode::Exact);
                    dayIconDrawn = true;
                }
            }

            if (!dayIconDrawn && dayLookupCode.length() > 0) {
                String iconPath = "/icons/" + dayLookupCode + ".bmp";
                if (LittleFS.exists(iconPath)) {
                    drawBmp(iconPath, centerX - 16, iconY);
                    dayIconDrawn = true;
                }
            }

            if (!dayIconDrawn) {
                drawNoIconPlaceholder(centerX - 18, iconY - 7, 36, 36);
            }


            String nightLookupCode = normalizeWeatherIconCode(currentForecast[i].icon_night);
            bool nightIconDrawn = false;
            if (nightLookupCode.length() > 0) {
                const unsigned char* iconData = getSmallIconData(nightLookupCode);
                if (iconData) {
                    drawIconFromProgmem(iconData, centerX - 18, startY + 104, 36, 36, IconRenderMode::Exact);
                    nightIconDrawn = true;
                }
            }

            if (!nightIconDrawn && nightLookupCode.length() > 0) {
                String iconPath = "/icons/" + nightLookupCode + ".bmp";
                if (LittleFS.exists(iconPath)) {
                    drawBmp(iconPath, centerX - 16, startY + 104);
                    nightIconDrawn = true;
                }
            }

            if (!nightIconDrawn) {
                drawNoIconPlaceholder(centerX - 18, startY + 104, 36, 36);
            }


        }


        if (count > 1) {
            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            for (int i = 1; i < currentForecastCount && i < 6; i++) {

                 int yH1 = chartBottom - ((highs[i] - minTemp) * chartHeight / tempRange);
                 int yH2 = chartBottom - ((highs[i+1] - minTemp) * chartHeight / tempRange);
                 paint_gfx.drawLine(xCoords[i], yH1, xCoords[i+1], yH2, 1);


                 int yL1 = chartBottom - ((lows[i] - minTemp) * chartHeight / tempRange);
                 int yL2 = chartBottom - ((lows[i+1] - minTemp) * chartHeight / tempRange);
                 paint_gfx.drawLine(xCoords[i], yL1, xCoords[i+1], yL2, 1);


                 paint_gfx.fillCircle(xCoords[i], yH1, 2, 1);
                 paint_gfx.fillCircle(xCoords[i], yL1, 2, 1);

                 String hStr = String(highs[i]);
                 int hw = u8g2.getUTF8Width(hStr.c_str());
                 u8g2.drawUTF8(xCoords[i] - hw/2, yH1 - 5, hStr.c_str());

                 String lStr = String(lows[i]);
                 int lw = u8g2.getUTF8Width(lStr.c_str());
                 u8g2.drawUTF8(xCoords[i] - lw/2, yL1 + 12, lStr.c_str());


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


        epdFlushFrame(partial_update, BlackImage);
        hibernateEPD();


        if (sendSignal) {
            Serial2.println("bye");



            if (digitalRead(MODE_PIN) == HIGH) {
                digitalWrite(BYE_SIGNAL_PIN, LOW);
                Serial.println("Sent 'bye' via Serial2 and BYE_SIGNAL_PIN set to LOW (Battery Mode)");
            } else {
                Serial.println("BYE Signal Blocked (DC Mode)");
            }
        }




    }
}

void hibernateEPD() {
    Serial.println("Hibernating EPD...");
    int driver = getActiveDisplayDriver();
    if (driver == DISPLAY_DRIVER_LOCAL) {
#if ENABLE_DRIVER_LOCAL

        Local_EPD_4IN2_Sleep();
#endif
    } else {
#if ENABLE_DRIVER_GX2IC
        gxDisplay.hibernate();
#endif
    }


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

          if (jsonHasKey(doc, "date")) {
              JsonObject dateObj = doc["date"];
              if (jsonHasKey(dateObj, "solar")) solarDate = dateObj["solar"].as<String>();
              if (jsonHasKey(dateObj, "week")) weekDay = dateObj["week"].as<String>();
              if (jsonHasKey(dateObj, "lunar")) lunarDate = dateObj["lunar"].as<String>();
              if (jsonHasKey(dateObj, "term")) termInfo = dateObj["term"].as<String>();
              updateDatePending = true;
          }


          if (jsonHasKey(doc, "weather")) {
              JsonArray forecastArr = doc["weather"].as<JsonArray>();
              int count = 0;
              for (JsonVariant v : forecastArr) {
                  if (count >= 7) break;

                  if (jsonHasKey(v, "temp")) currentForecast[count].temp = v["temp"].as<String>();


                  if (jsonHasKey(v, "icon")) currentForecast[count].icon_day = v["icon"].as<String>();
                  else if (jsonHasKey(v, "iconDay")) currentForecast[count].icon_day = v["iconDay"].as<String>();
                  else if (jsonHasKey(v, "icon_day")) currentForecast[count].icon_day = v["icon_day"].as<String>();


                  if (jsonHasKey(v, "iconNight")) currentForecast[count].icon_night = v["iconNight"].as<String>();
                  else if (jsonHasKey(v, "icon_night")) currentForecast[count].icon_night = v["icon_night"].as<String>();


                  if (jsonHasKey(v, "cond")) currentForecast[count].cond_day = v["cond"].as<String>();
                  else if (jsonHasKey(v, "textDay")) currentForecast[count].cond_day = v["textDay"].as<String>();
                  else if (jsonHasKey(v, "text")) currentForecast[count].cond_day = v["text"].as<String>();
                  else if (jsonHasKey(v, "weather")) currentForecast[count].cond_day = v["weather"].as<String>();


                  if (jsonHasKey(v, "textNight")) currentForecast[count].cond_night = v["textNight"].as<String>();
                  else if (jsonHasKey(v, "text_night")) currentForecast[count].cond_night = v["text_night"].as<String>();
                  else if (jsonHasKey(v, "cond_night")) currentForecast[count].cond_night = v["cond_night"].as<String>();

                  if (jsonHasKey(v, "windDir")) currentForecast[count].wind_dir = v["windDir"].as<String>();
                  if (jsonHasKey(v, "windScale")) currentForecast[count].wind_sc = v["windScale"].as<String>();
                  if (jsonHasKey(v, "date")) currentForecast[count].date = v["date"].as<String>();

                  count++;
              }
              currentForecastCount = count;
              updateWeatherPending = true;
              fullRefreshPending = true;
          }


          if (jsonHasKey(doc, "env")) {
              JsonObject envObj = doc["env"];
              if (jsonHasKey(envObj, "temp")) indoorTemp = envObj["temp"].as<String>();
              if (jsonHasKey(envObj, "humi")) indoorHumi = envObj["humi"].as<String>();
              updateEnvPending = true;
          }


          if (jsonHasKey(doc, "air")) {
              JsonObject airObj = doc["air"];
              if (jsonHasKey(airObj, "pm2p5")) airPm2p5 = airObj["pm2p5"].as<String>();
              if (jsonHasKey(airObj, "category")) airCategory = airObj["category"].as<String>();
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
          if (jsonHasKey(doc, "阳历日期")) solarDate = doc["阳历日期"].as<String>();
          if (jsonHasKey(doc, "星期")) weekDay = doc["星期"].as<String>();
          if (jsonHasKey(doc, "农历日期")) lunarDate = doc["农历日期"].as<String>();
          if (jsonHasKey(doc, "节气信息")) termInfo = doc["节气信息"].as<String>();

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
                if (jsonHasKey(doc, "temp")) indoorTemp = doc["temp"].as<String>();
                if (jsonHasKey(doc, "humi")) indoorHumi = doc["humi"].as<String>();

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
                 if (jsonHasKey(doc, "pm2p5")) airPm2p5 = doc["pm2p5"].as<String>();
                 if (jsonHasKey(doc, "category")) airCategory = doc["category"].as<String>();
                 updateWeatherPending = true;
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

          std::vector<ShiftEvent> newShifts;


          time_t now = timeClient.getEpochTime();
          struct tm *t_now = gmtime(&now);
          int currentYear = t_now->tm_year + 1900;
          int currentMonth = t_now->tm_mon + 1;


          auto addTempShift = [&](int y, int m, int d, String c) {
              ShiftEvent ev;
              ev.year = y; ev.month = m; ev.day = d; ev.content = c;
              newShifts.push_back(ev);
          };


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
                           if (jsonHasKey(obj, "date")) dateStr = obj["date"].as<String>();
                           else if (jsonHasKey(obj, "day")) dateStr = obj["day"].as<String>();

                           if (dateStr.length() > 0) {
                               int year, month, day;
                               if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                                   String content = "";
                                   if (jsonHasKey(obj, "shift")) content = obj["shift"].as<String>();
                                   else if (jsonHasKey(obj, "content")) content = obj["content"].as<String>();
                                   else if (jsonHasKey(obj, "summary")) content = obj["summary"].as<String>();

                                   if (content.length() > 0) addTempShift(year, month, day, content);
                               }
                           }
                       }
                   }
               }
          }

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
                   if (jsonHasKey(root, "date") || jsonHasKey(root, "day")) {
                        String dateStr = jsonHasKey(root, "date") ? root["date"].as<String>() : root["day"].as<String>();
                        int year, month, day;
                        if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                            String content = "";
                            if (jsonHasKey(root, "shift")) content = root["shift"].as<String>();
                            else if (jsonHasKey(root, "content")) content = root["content"].as<String>();
                            if (content.length() > 0) addTempShift(year, month, day, content);
                        }
                   }
                   else {
                       JsonArray shifts;
                       if (jsonHasKey(root, "shifts")) shifts = root["shifts"];
                       else if (jsonHasKey(root, "data")) shifts = root["data"];

                       if (!shifts.isNull()) {
                           for (JsonVariant v : shifts) {
                               if (v.is<JsonObject>()) {
                                   JsonObject obj = v.as<JsonObject>();
                                   String dateStr = "";
                                   if (jsonHasKey(obj, "date")) dateStr = obj["date"].as<String>();

                                   int year, month, day;
                                   if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                                       String content = "";
                                       if (jsonHasKey(obj, "shift")) content = obj["shift"].as<String>();
                                       if (content.length() > 0) addTempShift(year, month, day, content);
                                   }
                               }
                           }
                       }
                   }
               }
          }






          if (!newShifts.empty()) {

              ShiftEvent minEv = newShifts[0];
              for(const auto& ev : newShifts) {
                  if (ev.year < minEv.year) minEv = ev;
                  else if (ev.year == minEv.year && ev.month < minEv.month) minEv = ev;
                  else if (ev.year == minEv.year && ev.month == minEv.month && ev.day < minEv.day) minEv = ev;
              }


              auto it = std::remove_if(shiftEvents.begin(), shiftEvents.end(), [&](const ShiftEvent& ev) {
                  if (ev.year > minEv.year) return true;
                  if (ev.year == minEv.year && ev.month > minEv.month) return true;
                  if (ev.year == minEv.year && ev.month == minEv.month && ev.day >= minEv.day) return true;
                  return false;
              });
              shiftEvents.erase(it, shiftEvents.end());


              shiftEvents.insert(shiftEvents.end(), newShifts.begin(), newShifts.end());

              Serial.printf("Merged %d new shifts. Min Date: %d-%d-%d\n", newShifts.size(), minEv.year, minEv.month, minEv.day);
          }


          std::sort(shiftEvents.begin(), shiftEvents.end(), [](const ShiftEvent& a, const ShiftEvent& b) {
              if (a.year != b.year) return a.year < b.year;
              if (a.month != b.month) return a.month < b.month;
              return a.day < b.day;
          });


          while (shiftEvents.size() > MAX_SHIFT_EVENTS) {
              shiftEvents.erase(shiftEvents.begin());
          }

          Serial.print("Total shifts stored: ");
          Serial.println(shiftEvents.size());

          updateWeatherPending = true;
          lastUpdateTrigger = millis();
      } else {
          Serial.print("JSON Error (Shift): ");
          Serial.println(error.c_str());
      }
      return;
  }

  if (strcmp(topic, "epd/weatherrequest") == 0) {

       return;
  }

  if (strcmp(topic, config.mqtt_weather_topic) == 0) {
      Serial.println("Weather updated");

      JsonDocument doc;

      JsonDocument filter;

      filter[0]["fxDate"] = true; filter[0]["date"] = true; filter[0]["日期"] = true;
      filter[0]["tempMax"] = true; filter[0]["tempMin"] = true; filter[0]["temp"] = true; filter[0]["最高温"] = true; filter[0]["最低温"] = true;
      filter[0]["textDay"] = true; filter[0]["textNight"] = true; filter[0]["白天天气"] = true; filter[0]["夜晚天气"] = true; filter[0]["天气"] = true; filter[0]["weather"] = true;
      filter[0]["iconDay"] = true; filter[0]["iconNight"] = true;
      filter[0]["windDirDay"] = true; filter[0]["windDir"] = true; filter[0]["风向"] = true;
      filter[0]["windScaleDay"] = true; filter[0]["windScale"] = true; filter[0]["windSpeedDay"] = true; filter[0]["风速"] = true;


      filter["daily"] = filter;
      filter["data"] = filter;
      filter["forecast"] = filter;
# 1754 "D:/ink/E-Paper_weather/e_weather/e_weather.ino"
      JsonDocument f;
      JsonObject p = f.to<JsonObject>();

      JsonObject e = p["e"].to<JsonObject>();
      e["fxDate"] = true; e["date"] = true; e["日期"] = true;
      e["tempMax"] = true; e["tempMin"] = true; e["temp"] = true; e["最高温"] = true; e["最低温"] = true;
      e["textDay"] = true; e["textNight"] = true; e["白天天气"] = true; e["夜晚天气"] = true; e["天气"] = true; e["weather"] = true;
      e["iconDay"] = true; e["iconNight"] = true;
      e["windDirDay"] = true; e["windDir"] = true; e["风向"] = true;
      e["windScaleDay"] = true; e["windScale"] = true; e["windSpeedDay"] = true; e["风速"] = true;


      filter[0] = e;
      filter["daily"][0] = e;
      filter["data"][0] = e;
      filter["forecast"][0] = e;

      DeserializationError error = deserializeJson(doc, (char*)payload, length, DeserializationOption::Filter(filter));

      if (!error) {
          Serial.println("Parsing JSON...");

          int count = 0;


          if (doc.is<JsonArray>()) {
              Serial.println("Found root array data");
              JsonArray data = doc.as<JsonArray>();
              for(JsonVariant v : data) {
                  if (count >= 7) break;


                  if (jsonHasKey(v, "fxDate")) currentForecast[count].date = v["fxDate"].as<String>();
                  else if (jsonHasKey(v, "日期")) currentForecast[count].date = v["日期"].as<String>();
                  else if (jsonHasKey(v, "date")) currentForecast[count].date = v["date"].as<String>();


                  if (jsonHasKey(v, "tempMax") && jsonHasKey(v, "tempMin")) {
                      currentForecast[count].temp = v["tempMax"].as<String>() + "/" + v["tempMin"].as<String>();
                  } else if (jsonHasKey(v, "最高温") && jsonHasKey(v, "最低温")) {
                      currentForecast[count].temp = v["最高温"].as<String>() + "/" + v["最低温"].as<String>();
                  } else if (jsonHasKey(v, "temp")) {
                      currentForecast[count].temp = v["temp"].as<String>();
                  }


                  if (jsonHasKey(v, "textDay")) currentForecast[count].cond_day = v["textDay"].as<String>();
                  else if (jsonHasKey(v, "白天天气")) currentForecast[count].cond_day = v["白天天气"].as<String>();
                  else if (jsonHasKey(v, "天气")) currentForecast[count].cond_day = v["天气"].as<String>();
                  else if (jsonHasKey(v, "weather")) currentForecast[count].cond_day = v["weather"].as<String>();


                  if (jsonHasKey(v, "textNight")) currentForecast[count].cond_night = v["textNight"].as<String>();
                  else if (jsonHasKey(v, "夜晚天气")) currentForecast[count].cond_night = v["夜晚天气"].as<String>();


                  if (jsonHasKey(v, "iconDay")) currentForecast[count].icon_day = v["iconDay"].as<String>();
                  if (jsonHasKey(v, "iconNight")) currentForecast[count].icon_night = v["iconNight"].as<String>();


                  if (jsonHasKey(v, "windDirDay")) currentForecast[count].wind_dir = v["windDirDay"].as<String>();
                  else if (jsonHasKey(v, "windDir")) currentForecast[count].wind_dir = v["windDir"].as<String>();
                  else if (jsonHasKey(v, "风向")) currentForecast[count].wind_dir = v["风向"].as<String>();

                  if (jsonHasKey(v, "windScaleDay")) currentForecast[count].wind_sc = v["windScaleDay"].as<String>();
                  else if (jsonHasKey(v, "windScale")) currentForecast[count].wind_sc = v["windScale"].as<String>();
                  else if (jsonHasKey(v, "windSpeedDay")) currentForecast[count].wind_sc = v["windSpeedDay"].as<String>();
                  else if (jsonHasKey(v, "风速")) currentForecast[count].wind_sc = v["风速"].as<String>();

                  if (currentForecast[count].date.length() > 0) {
                      count++;
                  }
              }
          }

          else {
              JsonArray data = doc["data"];
              if (data.isNull()) data = doc["daily"];
              if (data.isNull()) data = doc["forecast"];

              if (!data.isNull()) {
                  Serial.println("Found nested array data");
                  for(JsonVariant v : data) {
                      if (count >= 7) break;

                      if (jsonHasKey(v, "date")) currentForecast[count].date = v["date"].as<String>();
                      else if (jsonHasKey(v, "day")) currentForecast[count].date = v["day"].as<String>();

                      if (jsonHasKey(v, "temp")) currentForecast[count].temp = v["temp"].as<String>();
                      else if (jsonHasKey(v, "high")) currentForecast[count].temp = v["high"].as<String>();

                      if (jsonHasKey(v, "weather")) currentForecast[count].cond_day = v["weather"].as<String>();
                      else if (jsonHasKey(v, "text")) currentForecast[count].cond_day = v["text"].as<String>();

                      if (currentForecast[count].date.length() > 0) count++;
                  }
              }
          }

          Serial.print("Forecast count: ");
          Serial.println(count);
          currentForecastCount = count;

          if (count > 0) {
              updateWeatherPending = true;
              fullRefreshPending = true;
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
# 1967 "D:/ink/E-Paper_weather/e_weather/e_weather.ino"
void handleButtonClick() {
    Serial.println("Click: Refreshing Weather Page...");
    displayWeatherDashboard(false);
}

void handleButtonDoubleClick() {
    Serial.println("Double Click: Manual sleep triggered");
    enterDeepSleep();
}

void handleButtonLongPress() {
    Serial.println("Long Press: Refreshing Weather Page...");
    displayWeatherDashboard(false);
}

unsigned long lastMqttRetry = 0;
int mqttRetryCounter = 0;
bool wifiWarningShown = false;
bool mqttWarningShown = false;
bool mqttGiveUp = false;

void setup() {
  Serial.begin(115200);
  delay(100);
  printf("EPD_4IN2 WiFi Demo\r\n");
  DEV_Module_Init();


  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);


  pinMode(BYE_SIGNAL_PIN, OUTPUT);
  digitalWrite(BYE_SIGNAL_PIN, HIGH);


  pinMode(ADC_SWITCH_EN_PIN, OUTPUT);
  setAdcMeasurementPower(false);


  pinMode(MODE_PIN, INPUT);
  delay(100);

  int modeState = digitalRead(MODE_PIN);
  Serial.printf("Mode Pin (GPIO %d) State: %d (%s)\n", MODE_PIN, modeState, (modeState == HIGH) ? "HIGH" : "LOW");


  loadConfig();


  if (modeState == HIGH) {
      esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
      if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
          Serial.println("Wakeup: Timer (Battery Mode). Auto-refresh and sleep.");
          configMode = false;
      } else {

          Serial.println("Wakeup: User/Power-on (Battery Mode). Staying awake for Web UI.");
          configMode = true;
          wakeTime = millis();
      }
  } else {
      Serial.println("Mode: DC Powered. Staying awake.");
      configMode = true;
  }


  float currentVoltage = getBatteryVoltage();
  if (modeState == HIGH && currentVoltage > 0.1 && currentVoltage < config.low_battery_threshold) {
      Serial.printf("Low Battery Detected: %.2fV (Threshold: %.2fV)\n", currentVoltage, config.low_battery_threshold);
      String warnMsg = "LOW BATTERY WARNING!\nVoltage: " + String(currentVoltage, 2) + "V\nPlease charge the device.";
      displayMessageWithBitmap(warnMsg, getOtherIconData("ink"), 128, 128);



      while(1) {
          delay(1000);
      }
  }

  Serial.print("Air Quality Topic: ");
  Serial.println(config.mqtt_air_quality_topic);


  Serial2.begin(9600, SERIAL_8N1, 16, 17);






  bool enableAP = false;

  if (strlen(config.wifi_ssid) > 0) {
      WiFi.mode(WIFI_STA);


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
      while (WiFi.status() != WL_CONNECTED && retry < 60) {
          delay(500);
          retry++;
      }

      if (WiFi.status() == WL_CONNECTED) {
          Serial.println("WiFi Connected!");
          Serial.print("IP Address: ");
          Serial.println(WiFi.localIP());


          if (modeState == LOW) {
              String networkInfo = "WiFi Connected!\n";
              networkInfo += "IP: " + WiFi.localIP().toString() + "\n";
              networkInfo += "GW: " + WiFi.gatewayIP().toString();
              displayMessageWithBitmap(networkInfo, getOtherIconData("ink"), 128, 128);
              delay(2000);
          }

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

      uint8_t mac[6];
      WiFi.macAddress(mac);

      char macSuffix[5];
      sprintf(macSuffix, "%02X%02X", mac[4], mac[5]);

      const char* baseSSID = (strlen(config.device_name) > 0) ? config.device_name : DEFAULT_AP_SSID_BASE;
      ap_ssid = String(baseSSID) + "-" + String(macSuffix);

      WiFi.softAP(ap_ssid.c_str());
      Serial.println("AP Started: " + ap_ssid);
      displayMessage("WiFi Timeout!\nAP Started: " + ap_ssid + "\nIP: 192.168.4.1");
  }
# 2140 "D:/ink/E-Paper_weather/e_weather/e_weather.ino"
  if (strlen(config.mqtt_server) > 0 && WiFi.status() == WL_CONNECTED) {
      client.setServer(config.mqtt_server, config.mqtt_port);
      client.setCallback(mqttCallback);
      client.setBufferSize(4096);


      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);

      int mqttRetry = 0;
      bool mqttConnected = false;
      while (mqttRetry < 30 && !mqttConnected) {
          if (client.connect(clientId.c_str(), config.mqtt_user, config.mqtt_pass)) {
              mqttConnected = true;
          } else {
              delay(500);
              mqttRetry++;
          }
      }

      if (mqttConnected) {
          Serial.println("MQTT Connected (Setup)");


          if (enableAP) {
              WiFi.softAPdisconnect(true);
              WiFi.mode(WIFI_STA);
              Serial.println("MQTT Connected! AP Disabled.");
          }


          const char* ntp1 = (strlen(config.ntp_server) > 0) ? config.ntp_server : "ntp.aliyun.com";
          const char* ntp2 = (strlen(config.ntp_server_2) > 0) ? config.ntp_server_2 : "ntp.tencent.com";

          timeClient.setPoolServerName(ntp1);
          timeClient.setTimeOffset(28800);
          timeClient.begin();

          Serial.printf("Waiting for NTP (%s)...", ntp1);

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
          client.subscribe(config.mqtt_shift_topic);
          client.subscribe(config.mqtt_air_quality_topic);
          client.subscribe(config.mqtt_unified_topic);

          client.loop();
          delay(100);


          if (client.publish(config.mqtt_request_topic, "get")) {
              Serial.println("Weather Request sent (Setup)");
          } else {
              Serial.println("Weather Request failed (Setup)");
          }


          publishBatteryVoltage();
      } else {
          Serial.print("MQTT Connect failed, rc=");
          Serial.println(client.state());
          displayMessage("MQTT Connect failed!\nCheck Broker Config.");
      }
  }



  server.on("/", handleRoot);
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

  server.on("/delete", handleDelete);
  server.begin();

  printf("HTTP server started\r\n");


  const char* hostName = (strlen(config.device_name) > 0) ? config.device_name : "EPD-Display";
  ArduinoOTA.setHostname(hostName);

  if (MDNS.begin(hostName)) {
      Serial.printf("mDNS responder started: http://%s.local\n", hostName);
      MDNS.addService("http", "tcp", 80);
  }

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
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


  button.attachClick(handleButtonClick);
  button.attachDoubleClick(handleButtonDoubleClick);
  button.attachLongPressStart(handleButtonLongPress);



  if (WiFi.status() == WL_CONNECTED) {
      wifiWarningShown = false;
  } else {
      wifiWarningShown = true;
  }
}

void loop() {
  button.tick();
  server.handleClient();


  if (strlen(config.wifi_ssid) > 0) {
      if (WiFi.status() != WL_CONNECTED) {
          if (!wifiWarningShown) {
              Serial.println("WiFi Lost, showing warning");
              displayMessage("WiFi Connection Lost\nAP Enabled: " + ap_ssid);
              wifiWarningShown = true;


              if ((WiFi.getMode() & WIFI_AP) == 0) {
                   WiFi.mode(WIFI_AP_STA);
                   WiFi.softAP(ap_ssid.c_str());
              }
          }

      } else {
          if (wifiWarningShown) {
              Serial.println("WiFi Restored");
              wifiWarningShown = false;
          }
      }
  }


  if (mqttGiveUp) {
      if (!mqttWarningShown) {
           Serial.println("MQTT Failed (Persistent). AP Enabled.");

           if ((WiFi.getMode() & WIFI_AP) == 0) {
                WiFi.mode(WIFI_AP_STA);
                WiFi.softAP(ap_ssid.c_str());
           }
           String msg = "MQTT Connect Failed\nConfig via AP: " + ap_ssid + "\nOr IP: " + WiFi.localIP().toString();
           displayMessage(msg);
           mqttWarningShown = true;
      }

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

                       WiFi.softAPdisconnect(true);
                       WiFi.mode(WIFI_STA);
                  } else {
                       mqttRetryCounter++;
                       Serial.printf("MQTT Reconnect failed (%d/3)\n", mqttRetryCounter);

                       if (mqttRetryCounter >= 3 && !mqttWarningShown) {
                            displayMessage("MQTT Connection Failed\nBroker not reachable.");
                            mqttWarningShown = true;


                       }
                  }
              }
          }
      } else {
          client.loop();
          mqttRetryCounter = 0;
          mqttGiveUp = false;
          mqttWarningShown = false;
      }
  }


  if (millis() - lastUpdateTrigger > UPDATE_DELAY_MS) {
      if (updateWeatherPending || updateEnvPending || updateDatePending) {
          Serial.printf("Triggering Deferred Weather Update (Full: %s)\n", fullRefreshPending ? "Yes" : "No");
          displayWeatherDashboard(!fullRefreshPending, updateWeatherPending);
          refreshDone = true;
          lastRefreshTime = millis();
          updateWeatherPending = false;
          updateEnvPending = false;
          updateDatePending = false;
          fullRefreshPending = false;
      }
  }


  static unsigned long lastNtpRetry = 0;
  static int ntpRetryCount = 0;
  static bool usingSecondaryNtp = false;

  if (client.connected()) {
      if (!timeClient.isTimeSet()) {
          if (millis() - lastNtpRetry > 5000) {
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

          if (config.ui_mode == 0) {
              timeClient.update();
          }
      }
  }


  static unsigned long lastFullRefreshTime = millis();
  if (config.full_refresh_period > 0) {
      if (millis() - lastFullRefreshTime > (unsigned long)config.full_refresh_period * 60 * 1000) {
          lastFullRefreshTime = millis();
          Serial.println("Custom Full Refresh Triggered");
          displayWeatherDashboard(false);
      }
  }


   static unsigned long lastRequestTime = millis();
   if (config.request_interval > 0 && client.connected()) {
       if (millis() - lastRequestTime > (unsigned long)config.request_interval * 60 * 1000) {
           lastRequestTime = millis();
           Serial.println("Sending Periodic Weather Request & Battery Update");
           client.publish(config.mqtt_request_topic, "get");
           publishBatteryVoltage();
       }
   }


  int currentMinute = timeClient.getMinutes();
  if (lastMinute != -1 && currentMinute != lastMinute) {

      int currentHour = timeClient.getHours();


      if (currentHour == 0 && currentMinute == 0) {
          Serial.println("Midnight! Refreshing page...");
          displayWeatherDashboard(false);
      } else {

          if (config.ui_mode == 0 && currentForecastCount > 0) {
              Serial.println("Minute changed, updating display (Partial)...");
              displayWeatherDashboard(true);
          }
      }
  }
  lastMinute = currentMinute;


  if (digitalRead(MODE_PIN) == HIGH) {

      if (configMode) {
          if (millis() - wakeTime > (unsigned long)config.config_timeout * 60000) {
              Serial.println("Config mode timeout. Entering sleep...");
              enterDeepSleep();
          }
      }

      else {

          if (refreshDone && (millis() - lastRefreshTime > (unsigned long)config.sleep_delay * 1000)) {
              Serial.println("Update cycle complete. Entering sleep...");
              enterDeepSleep();
          }


          if (millis() > 120000) {
              Serial.println("Failsafe: Awake too long without refresh. Sleeping...");
              enterDeepSleep();
          }
      }
  }
}

void enterDeepSleep() {
    Serial.println("Entering Deep Sleep Mode (Extreme Savings)...");


    pinMode(LED_PIN, INPUT);
    pinMode(MODE_PIN, INPUT);
    pinMode(BYE_SIGNAL_PIN, INPUT);
    setAdcMeasurementPower(false);
    pinMode(ADC_SWITCH_EN_PIN, INPUT);
    if (config.adc_pin >= 0) pinMode(config.adc_pin, INPUT);


    if (client.connected()) client.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();


    hibernateEPD();




    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);


    if (config.request_interval > 0) {
        uint64_t sleepTime = (uint64_t)config.request_interval * 60 * 1000000ULL;
        esp_sleep_enable_timer_wakeup(sleepTime);
        Serial.printf("Timer wakeup scheduled in %d minutes\n", config.request_interval);
    }
# 2542 "D:/ink/E-Paper_weather/e_weather/e_weather.ino"
    Serial.println("Final UART shutdown...");
    Serial.flush();
    Serial2.flush();
    delay(10);

    Serial.end();
    Serial2.end();

    pinMode(1, INPUT);
    pinMode(3, INPUT);
    pinMode(16, INPUT);
    pinMode(17, INPUT);


    delay(100);
    esp_deep_sleep_start();
}