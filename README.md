# Water Sensor

This sketch targets an ESP8266-based water sensor controller with:

- persistent runtime config in `LittleFS`
- a setup access point for first-time configuration and recovery
- local Wi-Fi access after valid station credentials are saved
- a live dashboard with current measurements and a recent-readings graph
- runtime-editable hardware, debug, filter, and network settings
- WebSocket telemetry for live UI updates without page polling

## Libraries

Install these Arduino packages:

- Board Manager package: `esp8266:esp8266`
  Arduino IDE label: `esp8266 by ESP8266 Community`
- Included with that ESP8266 core: `ESP8266WiFi`, `ESP8266WebServer`, `LittleFS`, `Hash`
- Library Manager package: `WebSockets`
  Used by this project via `WebSocketsServer.h`
- Library Manager package: `Arduino_JSON`
- Library Manager package: `SimpleKalmanFilter`

## First Run

1. Upload the sketch.
2. Upload the full `data/` filesystem image to LittleFS so `index.html`, `app.js`, `style.css`, and `config.json` are available on the device.
3. Connect to the setup AP named `WaterSensorSetup`.
4. Open the device page at `http://10.0.0.47/`.
5. Review the live dashboard, graph, and editable settings.
6. Enter your local Wi-Fi SSID/password and any water-level, filter, hardware, or debug values you want.
7. Save the settings. Station and AP network changes are applied live. Pin and serial changes are saved immediately but require a reboot from the dashboard before they take effect.

## Config File

The runtime settings are stored at `/config.json` on LittleFS. If the file is missing or invalid, the sketch recreates it from built-in defaults.

The dashboard intentionally does not echo stored Wi-Fi passwords back to the browser:

- leave a password field blank to keep the existing stored password
- enter a new password to replace it
- use the clear toggle to remove a stored password
