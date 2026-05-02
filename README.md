# Water Sensor

This sketch targets an ESP8266-based water sensor controller with:

- persistent runtime config in `LittleFS`
- a setup access point for first-time configuration and recovery
- local Wi-Fi access after valid station credentials are saved
- a built-in web form for tuning thresholds and Wi-Fi settings

## Libraries

Install these Arduino libraries/core components:

- `ESP8266` board support package
- `LittleFS` from the ESP8266 core
- `ESP8266WebServer` from the ESP8266 core
- `Arduino_JSON`
- `SimpleKalmanFilter`

## First Run

1. Upload the sketch.
2. Upload the `data/config.json` filesystem image to LittleFS.
3. Connect to the setup AP named `WaterSensorSetup`.
4. Open the device page at `http://192.168.4.1/`.
5. Enter your local Wi-Fi SSID/password and any water-level tuning values.
6. Save the form. The device will try to join your local LAN. If that fails, it falls back to the setup AP.

## Config File

The runtime settings are stored at `/config.json` on LittleFS. If the file is missing or invalid, the sketch recreates it from built-in defaults.
