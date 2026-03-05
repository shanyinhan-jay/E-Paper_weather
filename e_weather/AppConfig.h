#ifndef APP_CONFIG_H
#define APP_CONFIG_H

struct Config {
  char wifi_ssid[32] = "Linksys-2.4G-wifi5";
  char wifi_pass[32] = "19811201";
  char mqtt_server[64] = "broker.emqx.io";
  int  mqtt_port = 1883;
  char mqtt_user[32] = "";
  char mqtt_pass[32] = "";
  char mqtt_topic[64] = "shanyinhan/epd/text";
  char mqtt_weather_topic[64] = "shanyinhan/epd/weather";
  char mqtt_date_topic[64] = "shanyinhan/epd/date";
  char mqtt_env_topic[64] = "shanyinhan/epd/env";
  char mqtt_calendar_topic[64] = "shanyinhan/epd/calendar";
  char mqtt_shift_topic[64] = "shanyinhan/epd/shift";
  char mqtt_air_quality_topic[64] = "shanyinhan/epd/air_quality";
  char mqtt_unified_topic[64] = "shanyinhan/epd/unified"; // New unified topic
  char mqtt_request_topic[64] = "shanyinhan/epd/weatherrequest"; // New request topic
  char ntp_server[64] = "ntp.aliyun.com";
  char ntp_server_2[64] = "ntp.tencent.com";
  int full_refresh_period = 10; // minutes, 0 = disabled
  int request_interval = 30; // minutes, 0 = disabled
  int day_start_hour = 6;
  int day_end_hour = 18;
  bool invert_display = false;
  
  // Display Mode: 0 = Time Mode (Default), 1 = Date Mode
  int ui_mode = 1;
  int adc_pin = 34; // Default ADC pin for battery monitoring
  float adc_ratio = 2.0; // Voltage divider ratio (e.g., 2.0 for 100k/100k)
  float low_battery_threshold = 3.3; // Low battery warning threshold in Volts
  int sleep_delay = 10; // Seconds to stay awake after refresh in battery mode
  int config_timeout = 5; // Minutes to stay awake in Config Mode (after manual wakeup)
  
  // Static IP Configuration
  bool use_static_ip = false;
  char static_ip[16] = "";
  char static_gw[16] = "";
  char static_mask[16] = "255.255.255.0";
  char static_dns[16] = "114.114.114.114";
};

enum Page {
    PAGE_WEATHER,
    PAGE_CALENDAR
};

#endif
