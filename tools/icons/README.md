# 图标自动化流程

## 目标
- 保持 `7272` 与 `3636` 图标资源同步。
- 自动生成 `icon_m.h` 与 `icon_s.h`。
- 校验 MQTT 图标契约覆盖率（数组 + BMP 回退）。
- 让设计同学替图时无需改固件代码。

## 目录说明
- `e_weather/icon/7272`：72x72 天气图标数组。
- `e_weather/icon/3636`：36x36 天气图标数组。
- `e_weather/icon/icon_m.h`：自动生成，请勿手改。
- `e_weather/icon/icon_s.h`：自动生成，请勿手改。
- `tools/icons/mqtt_icon_codes.txt`：MQTT 图标代码契约清单。

## 一键流程
在项目根目录执行：

```bash
python tools/icons/gen_all_icons.py --compile
```

该命令会自动执行：
1. 扫描 `7272` 与 `3636` 图标目录。
2. 生成 `icon_m.h` 与 `icon_s.h`。
3. 输出两套目录代码差异告警。
4. 执行契约覆盖率校验：
   - MQTT code -> `7272` 数组
   - MQTT code -> `3636` 数组
   - MQTT code -> BMP 回退（`e_weather/icons/*.bmp`）
5. 编译固件。

## 严格 CI 模式
如有任何差异或覆盖率缺口即返回失败：

```bash
python tools/icons/gen_all_icons.py --strict --compile
```

## 设计交接流程
1. 在 `7272` 与 `3636` 中替换/新增对应图标数组文件。
2. 文件名与 MQTT 图标代码一致（例如 `350.c`）。
3. 执行一条命令：

```bash
python tools/icons/gen_all_icons.py --compile
```

4. 若报告中出现契约缺失，补齐图标文件或调整 `mqtt_icon_codes.txt`。
