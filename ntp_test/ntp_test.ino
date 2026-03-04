#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

// --- WiFi 配置 ---
const char* ssid = "Linksys-2.4G-wifi5";
const char* password = "19811201";

// --- NTPClient 配置 ---
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 28800, 60000);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ESP32 NTP 诊断测试 ===");

  // 1. WiFi 连接
  Serial.print("正在连接 WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(1000);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi 已连接!");
    Serial.print("IP 地址: ");
    Serial.println(WiFi.localIP());
    Serial.print("网关地址: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS 服务器: ");
    Serial.println(WiFi.dnsIP());
  } else {
    Serial.println("\nWiFi 连接失败! 请检查 SSID 和密码。");
    return;
  }

  // 2. 测试 NTPClient (当前代码使用的方法)
  Serial.println("\n--- 测试 NTPClient (库) ---");
  const char* servers[] = {"ntp.aliyun.com", "ntp.tencent.com", "cn.pool.ntp.org", "time.google.com"};
  
  for (int i = 0; i < 4; i++) {
    Serial.printf("尝试服务器: %s ... ", servers[i]);
    timeClient.setPoolServerName(servers[i]);
    
    if (timeClient.forceUpdate()) {
      unsigned long epoch = timeClient.getEpochTime();
      Serial.println("成功!");
      Serial.printf("  原始 Epoch: %lu\n", epoch);
      Serial.printf("  格式化时间: %s\n", timeClient.getFormattedTime().c_str());
      
      if (epoch < 1700000000) {
        Serial.println("  警告: Epoch 时间过旧, 可能是错误的!");
      }
    } else {
      Serial.println("失败!");
    }
    delay(1000);
  }

  // 3. 测试 ESP32 原生 configTime (推荐方法)
  Serial.println("\n--- 测试 ESP32 原生 configTime (推荐) ---");
  configTime(28800, 0, "192.168.1.249", "cn.pool.ntp.org");
  
  Serial.print("正在等待原生同步");
  struct tm timeinfo;
  retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 10) {
    Serial.print(".");
    delay(1000);
    retry++;
  }

  if (getLocalTime(&timeinfo)) {
    Serial.println("\n原生同步成功!");
    Serial.print("时间: ");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    
    time_t now;
    time(&now);
    Serial.printf("原始 Epoch: %ld\n", (long)now);
  } else {
    Serial.println("\n原生同步失败!");
  }

  Serial.println("\n诊断结束。");
}

void loop() {
  // 不做任何操作
}
