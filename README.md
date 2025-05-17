# Blueweather

基于 CH582 和 AHT20 的 BLEHome 温湿度传感器。

## 编译

使用 platformio `pio run` 即可。

## 使用

CH582 的 PB13 连 AHT20 的 SCL，PB12 连 SDA，PB7 供电（或者用 VCC 供电也可以）。

通过 WCH 官方 [WCHISPTOOL](https://www.wch.cn/downloads/wchisptool_setup_exe.html) 或者 [wchisp](https://github.com/ch32-rs/wchisp) 烧录即可。

### 蓝牙网关

推荐使用 ESP32-C3 烧录 ESPHome 作为蓝牙网关。

打开 [Bluetooth Proxy](https://esphome.io/components/bluetooth_proxy.html) 页面，将 ESP32-C3 接入电脑，按住 BOOT 不放，
再按 RESET 键，然后点击页面上的 Generic ESP32 进行烧录。

烧录完成后，再按一下 RESET 键，然后重新进入烧录界面，此时可以选择连接 WiFi，连上你的 HA 所在 WiFi。

打开 HA，应该会自动检测到 ESPHome 设备，连接即可。如果没有检测到，可以手动连一下。

### 接入 HA

有蓝牙网关后，HA 会自动发现 BTHome 设备，直接添加即可。

## License

由于使用了大量 WCH 例程，而 WCH 例程并没有开源 License，因此不知道怎么 License，
就简单地授权所有可以使用 WCH 例程的用户都可以以同等条件和方式使用本仓库的代码吧。
