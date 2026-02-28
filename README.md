# ESP32 4.2寸 墨水屏天气时钟 (E-Paper Weather Station)

基于 ESP32 和 4.2英寸电子墨水屏 (E-Paper Display) 开发的智能天气与信息看板。支持通过 MQTT 协议接入 Home Assistant、Node-RED 等智能家居平台，实现高度可定制的数据展示。

## 🌟 主要功能

### 1. 墨水屏显示
- **天气看板**: 显示当前日期、农历、节气、实时温湿度、空气质量以及未来6天的天气预报。
- **日历视图**: 提供月历视图，可显示日程安排或排班信息。
- **日夜模式**: 支持自定义日间和夜间模式的切换时间（例如 6:00 - 18:00 为白底黑字，夜间反转）。
- **图标支持**: 支持通过 Web 管理页面上传自定义 BMP 天气图标。

### 2. 网络连接
- **WiFi 连接**: 支持 STA 模式连接路由器。
- **自动配网 (AP 模式)**: 当 WiFi 连接失败时，自动开启热点（AP），方便用户通过手机连接并配置网络。
- **静态 IP 支持**: 可在 Web 页面配置静态 IP 地址。
- **NTP 时间同步**: 支持双 NTP 服务器配置，确保时间准确。

### 3. MQTT 数据集成
- **统一主题 (Unified Topic)**: 推荐使用。支持通过单条 JSON 消息下发所有数据（天气、环境、日历等），减少网络开销。
- **独立主题**: 兼容旧版，支持分别订阅天气、日期、环境等独立主题。
- **定时主动请求**: 支持配置定时任务，定期向指定 MQTT 主题（默认 `epd/weatherrequest`）发送请求指令，触发服务器端（如 Node-RED）推送最新数据。

### 4. Web 管理后台
内置嵌入式 Web 服务器，提供友好的图形化配置界面：
- **仪表盘**: 切换显示页面（天气/日历），发送测试文本。
- **系统配置**: 设置 WiFi、MQTT 服务器、NTP 服务器、静态 IP。
- **显示设置**: 配置全刷间隔（Full Refresh Period）、数据请求间隔（Request Interval）、日夜模式时间。
- **MQTT 配置**: 详细配置各类 MQTT 主题及 Unified Topic。
- **文件管理**: 上传和删除墨水屏使用的 BMP 图标文件。
- **固件升级 (OTA)**: 支持通过网页直接上传 `.bin` 文件进行固件升级。

## 🛠️ 硬件需求
- **主控**: ESP32 开发板 (如 ESP32 DevKitC)
- **显示屏**: 4.2 inch E-Paper Display (SPI 接口)
- **连接线**: 标准 SPI 连接 (BUSY, RST, DC, CS, CLK, DIN, GND, VCC)

## 📡 MQTT 通信协议

### 1. 统一消息格式 (推荐)
向配置的 `Unified Topic` 发送如下 JSON 数据：

```json
{
  "date": {
    "solar": "2023-10-27",
    "week": "星期五",
    "lunar": "九月十三",
    "term": "霜降"
  },
  "weather": [
    {
      "date": "2023-10-27",
      "temp": "22/15",
      "textDay": "晴",
      "iconDay": "100",
      "textNight": "晴",
      "iconNight": "150",
      "windDir": "西北风",
      "windScale": "3"
    },
    // ... 更多预报数据
  ],
  "env": {
    "temp": "25.5",
    "humi": "60"
  },
  "air": {
    "pm2p5": "25",
    "category": "优"
  }
}
```

### 2. 定时主动请求
- 设备会根据设定的 **Request Interval** 定期向 **Periodic Request Topic** 发送消息。
- **Payload**: `get`
- **用途**: 配合 Node-RED 的 `mqtt in` 节点，收到 `get` 指令后触发 HTTP 请求获取天气，并通过 MQTT 返回数据给设备。

## 📖 使用指南

1. **初次使用**:
   - 设备上电后，若无法连接 WiFi，将自动建立名为 `EPD-Config-AP` (或其他预设名称) 的热点。
   - 连接该热点，访问 `http://192.168.4.1` 进入配置页面。

2. **配置网络与 MQTT**:
   - 在 Web 页面填写 WiFi SSID、密码。
   - 填写 MQTT Broker 地址、端口、用户名密码。
   - 保存并重启。

3. **配置 MQTT 主题**:
   - 点击首页的 "Configure MQTT Topics" 按钮。
   - 设置 Unified Topic 或其他独立 Topic。
   - 设置 Periodic Request Topic（用于主动请求数据）。

4. **上传图标**:
   - 访问 "File Manager"，上传对应天气代码的 BMP 图标文件（如 `100.bmp` 对应晴天）。

## 📝 开发与编译
- 使用 **Visual Studio Code** + **PlatformIO** 或 **Arduino IDE** 进行开发。
- 依赖库: `Adafruit GFX`, `U8g2_for_Adafruit_GFX`, `ArduinoJson`, `LittleFS` 等。
- 确保分区表选择支持 LittleFS 的方案（如 `min_spiffs` 或自定义分区）。
