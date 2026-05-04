#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifndef DEFAULT_DISPLAY_DRIVER
#define DEFAULT_DISPLAY_DRIVER -1
#endif

struct Config {
  char wifi_ssid[32] = "Linksys-2.4G-wifi5";
  char wifi_pass[32] = "19811201";
  char mqtt_server[64] = "broker.emqx.io";
  int  mqtt_port = 1883;
  int  mqtt_protocol = 0; // 0 = MQTT, 1 = MQTT over TLS / MQTTS
  char mqtt_user[32] = "";
  char mqtt_pass[32] = "";
  char mqtt_hourly_topic[64] = "shanyinhan/epd/hourly";
  char mqtt_battery_topic[64] = "shanyinhan/epd/battery";
  char mqtt_unified_topic[64] = "shanyinhan/epd/unified";
  char ntp_server[64] = "ntp.aliyun.com";
  char ntp_server_2[64] = "ntp.tencent.com";
  int full_refresh_period = 10; // minutes, 0 = disabled
  int request_interval = 30; // minutes, 0 = periodic battery publish disabled
  int day_start_hour = 6;
  int day_end_hour = 18;
  bool invert_display = false;
  
  // Display Mode: 0 = Time Mode (Default), 1 = Date Mode
  int ui_mode = 1;
  // EPD Driver: -1 = Auto, 0 = Local_EPD_4IN2, 1 = GxEPD2_2IC
  int display_driver = DEFAULT_DISPLAY_DRIVER;
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

#endif
