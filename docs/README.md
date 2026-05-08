# Water Station Reservoir Filler

## Purpose

This project is an ESP8266-based reservoir water-level controller. The firmware samples an ultrasonic water-level sensor, filters the measured echo timing, drives a water relay output, exposes a browser dashboard, and provides a latched emergency shutoff control.

## Configuration Authority

`/config.json` on LittleFS is the runtime configuration authority. The firmware loads and validates this file before initializing runtime parameters, GPIO assignments, Wi-Fi station mode, setup AP mode, HTTP service, WebSocket service, DNS service, measurement filtering, and control timing.

The configuration reader parses the flat top-level JSON object field-by-field. This keeps the runtime configuration path independent of Arduino `JSONVar` object-entry capacity and preserves all configured Wi-Fi fields, including station credentials and setup AP parameters.

A valid `/config.json` permits runtime service initialization. Configuration-gated startup keeps network service and water-control service inactive while the serial console reports the configuration condition.

The packaged filesystem source is:

```text
/data/config.json
```

Upload the `data/` directory to LittleFS so the device contains:

```text
/index.html
/app.js
/style.css
/config.json
```

## Packaged Network Configuration

The packaged `/config.json` contains these network settings:

```json
{
  "wifiStaSsid": "MyGroove",
  "wifiStaPassword": "be84267e",
  "enableStationDhcp": false,
  "wifiStaIp": "10.0.0.53",
  "wifiStaGateway": "10.0.0.1",
  "wifiStaSubnet": "255.0.0.0",
  "wifiApSsid": "WaterSensorSetup",
  "wifiApPassword": "",
  "enableApDhcp": true,
  "wifiApIp": "10.0.0.47",
  "wifiApGateway": "10.0.0.47",
  "wifiApSubnet": "255.0.0.0",
  "wifiApChannel": 6,
  "wifiApHidden": false,
  "wifiApMaxConnections": 4,
  "wifiStaConnectTimeoutMs": 15000,
  "httpPort": 80,
  "websocketPort": 81,
  "dnsPort": 53
}
```

Station DHCP is disabled in the packaged configuration. Setup AP DHCP is enabled in the packaged configuration. Both settings are present in the dashboard and persist through `/api/config` into `/config.json`.

## Browser Access

When the ESP8266 operates as the setup AP, connect the computer to:

```text
WaterSensorSetup
```

Then open:

```text
http://10.0.0.47/
```

The boot log prints the active dashboard URL:

```text
Setup AP dashboard URL: http://<active-ap-ip>/
```

The browser URL uses `http://`. With AP DHCP enabled, the computer receives an AP-side address from the ESP8266. With AP DHCP disabled, the computer requires a manual IPv4 address compatible with the AP subnet.

## Boot Diagnostics

The boot console reports the configuration path before Wi-Fi initialization:

```text
LittleFS mounted
LittleFS root inventory:
  /config.json  <size> bytes
Loaded /config.json bytes: <size>
Runtime configuration loaded and validated.
```

A configuration-gated halt reports the configuration condition, for example:

```text
Configuration halt: /config.json is not present on LittleFS.
Runtime configuration is required at /config.json on LittleFS.
Network service and water-control service are inactive in this state.
```

The root inventory confirms that the LittleFS upload contains `/config.json`, `/index.html`, `/app.js`, and `/style.css`.

## Features

- Ultrasonic echo sampling with multi-ping validation.
- Kalman-filtered reservoir-level estimate.
- Water relay control from high/low/error thresholds.
- Error indicator output.
- Latched emergency shutoff.
- Browser dashboard served from LittleFS.
- Firmware-served compact dashboard when `/index.html`, `/app.js`, or `/style.css` assets are unavailable.
- JSON status endpoint at `/api/status`.
- Configuration save endpoint at `/api/config`.
- Emergency shutoff endpoint at `/api/emergency-stop`.
- Restart endpoint at `/api/restart`.
- WebSocket telemetry on the configured WebSocket port.
- Station Wi-Fi operation for LAN access.
- Setup AP operation for direct local access.
- Captive-portal DNS service on the configured DNS port while the setup AP is active.
- Password-preserving dashboard behavior for stored Wi-Fi credentials.

## Project Files

- `water_sensor.ino`: main setup loop, configuration-gated startup, runtime service loop, and restart handling.
- `src/common.h`: shared configuration, measurement, history, and runtime state declarations.
- `src/ip_utils.h` and `src/ip_utils.cpp`: IPv4 parsing helpers shared by configuration validation.
- `config.ino`: LittleFS configuration load/save, JSON parsing, and configuration validation.
- `network_ui.ino`: Wi-Fi station/AP control, AP DHCP control, captive DNS service, HTTP endpoints, WebSocket state publication, firmware dashboard serving, and dashboard API handling.
- `measurement.ino`: ultrasonic sampling, filtering, validity decisions, relay/error-output control, and emergency shutoff enforcement.
- `json_helpers.ino`: JSON escaping and serialization helpers.
- `data/index.html`: dashboard document.
- `data/app.js`: dashboard behavior, graph rendering, form handling, emergency shutoff request handling, and WebSocket client logic.
- `data/style.css`: dashboard styling.
- `data/config.json`: runtime configuration image for LittleFS upload.

## Arduino Dependencies

Install these Arduino packages:

- Board Manager package: `esp8266:esp8266`
  - Arduino IDE label: `esp8266 by ESP8266 Community`
- ESP8266 core libraries:
  - `ESP8266WiFi`
  - `ESP8266WebServer`
  - `DNSServer`
  - `LittleFS`
  - `Hash`
- Library Manager packages:
  - `WebSockets`, used through `WebSocketsServer.h`
  - `Arduino_JSON`
  - `SimpleKalmanFilter`

## First-Run Procedure

1. Open `water_sensor.ino` in the Arduino IDE or compile the sketch with the ESP8266 Arduino toolchain.
2. Select the correct ESP8266 board and port.
3. Upload the sketch.
4. Upload the `data/` directory to LittleFS.
5. Open serial output at `74880` baud during boot.
6. Confirm the boot log includes the LittleFS inventory and `Runtime configuration loaded and validated.`
7. Connect to `WaterSensorSetup` when station join is unavailable.
8. Open `http://10.0.0.47/` while connected to the setup AP.
9. Log in with the default HTTP Basic credentials:
   - Username: `admin`
   - Password: `admin`
10. Review live measurements, hardware settings, filter parameters, station Wi-Fi settings, setup AP settings, DHCP selections, and security settings.
11. Save the settings. Station and setup AP network changes are applied live. Pin, port, and serial-baud changes are saved immediately but require a reboot from the dashboard before they take effect.
12. Use the dashboard restart control when pin, port, or serial-baud settings are edited.

## Runtime Configuration Fields

The dashboard presents editable fields for:

- Measurement thresholds and timing.
- Sampling and filtering parameters.
- Debug logging controls.
- HTTP, WebSocket, and DNS ports.
- GPIO pin assignments.
- Serial baud rate.
- Station SSID and password.
- Station DHCP/static addressing.
- Station IP, gateway, and subnet.
- Setup AP SSID and password.
- Setup AP DHCP service.
- Setup AP IP, gateway, and subnet.
- Setup AP channel, SSID visibility, and maximum client count.
- Admin password management.

## Security

The dashboard is protected by HTTP Basic Authentication. The default username is `admin` and the default password is `admin`. Change the admin password in the Security section after first login.

The dashboard intentionally does not echo stored passwords (Wi-Fi or admin) back to the browser:

Every dashboard save posts the complete editable configuration to `/api/config`. The backend validates the candidate configuration, writes it to `/config.json`, and publishes the active state to the dashboard.

## Wi-Fi Operation

The firmware uses one Wi-Fi mode at a time:

- Station mode: `WiFi.mode(WIFI_STA)`.
- Setup AP mode: `WiFi.mode(WIFI_AP)`.

Station mode starts when `wifiStaSsid` contains a value and the station join completes within `wifiStaConnectTimeoutMs`. When `enableStationDhcp` is `false`, the station interface uses `wifiStaIp`, `wifiStaGateway`, and `wifiStaSubnet` from `/config.json`.

Setup AP mode starts when station join is unavailable. The AP interface uses `wifiApSsid`, `wifiApPassword`, `wifiApIp`, `wifiApGateway`, `wifiApSubnet`, `wifiApChannel`, `wifiApHidden`, `wifiApMaxConnections`, and `enableApDhcp` from `/config.json`.

The ESP8266 station interface supports 2.4 GHz Wi-Fi. Use a 2.4 GHz SSID with WPA/WPA2-Personal security.

## Dashboard Password Semantics

The dashboard stores Wi-Fi credentials through write-only password fields. Password fields represent write intent:

- Blank password field: preserve the stored password.
- Nonblank password field: store the entered password.
- Clear checkbox selected: remove the stored password.

This applies to both station and setup AP passwords.

## Emergency Shutoff

The dashboard header contains an **Emergency Shutoff** button. The button sends a `POST` request to:

```text
/api/emergency-stop
```

The endpoint applies a latched emergency state with these effects:

- `emergencyStopActive` becomes `true`.
- The water relay output is immediately forced off through `setWaterOutput(false)`.
- The filling state is cleared.
- The fill-duration counter is cleared.
- The error indicator is asserted.
- Live status JSON and WebSocket status publication report the emergency state.
- Automatic filling remains inhibited until the controller restarts.

A restart clears the volatile emergency latch because the latch is runtime state.

## Settings That Apply Immediately

The following settings are applied live when saved from the dashboard:

- Station SSID and station password.
- Station DHCP/static addressing selection.
- Station static IP, gateway, and subnet.
- Setup AP SSID, password, DHCP selection, IP, gateway, subnet, channel, SSID visibility, and client count.
- DNS port.
- Station connection timeout.
- Measurement thresholds, sampling parameters, filtering parameters, debug flags, and history capacity.

## Settings That Apply On Restart

The following low-level settings are stored immediately and used by the running firmware on the next boot:

- Trigger pin.
- Echo pin.
- Water relay pin.
- Error LED pin.
- HTTP port.
- WebSocket port.
- Serial baud rate.

## Serial Diagnostics

Boot and network diagnostics are printed on the serial port. During early boot, use `74880` baud. Once `/config.json` is loaded, serial output uses the configured `serialBaud` value.

Useful network diagnostics include:

- `LittleFS mounted`
- `Active runtime config`
- `Station DHCP enabled.`
- `Station static IP requested: 10.0.0.53`
- `Joining station SSID: MyGroove`
- `Station joined. IP: <address>`
- `Station join failed. Final status: <status>`
- `Starting setup AP.`
- `Setup AP DHCP enabled.`
- `Setup AP start result: success`
- `Setup AP SSID: WaterSensorSetup`
- `Setup AP IP: 10.0.0.47`
- `Setup AP dashboard URL: http://10.0.0.47/`

## AP Dashboard Checklist

Use this checklist when the computer associates with the setup AP and the dashboard page remains unavailable:

1. Confirm that the browser URL is `http://10.0.0.47/`.
2. Confirm that the computer is connected to `WaterSensorSetup` during browser access.
3. Confirm that the computer receives an AP-side IPv4 address when `enableApDhcp` is `true`.
4. Confirm that the computer has a manually assigned compatible IPv4 address when `enableApDhcp` is `false`.
5. Confirm that the browser is using plain HTTP.
6. Inspect the boot log for `Setup AP dashboard URL` and use the printed URL.
7. Confirm that LittleFS contains `/index.html`, `/app.js`, `/style.css`, and `/config.json`.
8. Open `/api/status` from the browser to distinguish HTTP service from dashboard asset loading.
