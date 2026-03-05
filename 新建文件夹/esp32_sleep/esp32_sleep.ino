#include <Arduino.h>
#include <WiFi.h>
#include <esp_bt.h>
#include <esp_sleep.h>

// 定义睡眠时间 (秒) 和 唤醒时间 (毫秒)
#define TIME_TO_SLEEP  20        // 睡眠20秒
#define AWAKE_TIME     5000      // 唤醒保持5秒 (5000毫秒)

// 微秒到秒的转换系数
#define uS_TO_S_FACTOR 1000000ULL 

// 打印唤醒原因的辅助函数
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("由定时器唤醒 (Timer)"); break;
    default:                     Serial.println("正常上电或复位唤醒"); break;
  }
}

void setup() {
  // 1. 初始化串口
  Serial.begin(115200);
  delay(500); // 给串口工具一点时间连接
  
  Serial.println("\n===========================");
  Serial.println("ESP32 启动!");
  print_wakeup_reason();

  // 2. 降低唤醒期间的功耗 (如果不需要网络和蓝牙)
  // 如果你的唤醒期间只是处理传感器数据，一定要显式关闭WiFi和蓝牙
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop(); // 关闭传统蓝牙
  // esp_bluedroid_disable(); 
  // esp_bt_controller_disable(); 

  // 3. 保持唤醒状态 5秒钟
  Serial.println("保持唤醒状态 5 秒钟...");
  delay(AWAKE_TIME); 

  // 4. 配置深度睡眠定时器
  // 注意：此处乘以 uS_TO_S_FACTOR 将秒转换为微秒
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.println("准备进入深度睡眠 (Deep Sleep)...");
  
  // 5. 【极其重要】确保串口数据发送完毕再休眠，否则最后一句打印会丢失
  Serial.flush(); 

  // 6. 进入深度睡眠
  // 调用此函数后，ESP32 将停止执行后续代码，RTC 外设保持运行
  esp_deep_sleep_start();
}

void loop() {
  // 在深度睡眠模式下，ESP32 唤醒后会重新从 setup() 开始执行
  // 因此 loop() 里面不需要写任何代码
}