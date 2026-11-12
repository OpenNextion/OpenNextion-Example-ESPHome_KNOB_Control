# OpenNextion ESPHome Knob Control

[简体中文](README.zh-CN.md)

ESPHome firmware project for the ONX2424G013 round knob display. It uses an ESP32-S3, ESP-IDF, LVGL, and bundled custom components to provide a local Home Assistant control panel.

## Demo

<p align="center">
  <img src="docs/control_ha_light.gif" alt="Control home-assistant light">
</p>

## Features

- Controls Home Assistant switches, lights, plugs, covers, and automations from the display.
- Supports rotary and push-button input, screen brightness, standby settings, and standby weather.
- Provides Wi-Fi captive-portal provisioning when no network has been configured.
- Supports HTTPS manifest-based firmware updates; the current release OTA image is in `firmware/`.

The main configuration is [ONX2424G013.yaml](ONX2424G013.yaml). The target hardware uses an ESP32-S3 with 16 MB flash, Octal PSRAM, and a GC9A01A round display; use this firmware only with compatible ONX2424G013 hardware.

## Home Assistant entity list

The display does not download the full Home Assistant entity registry. Instead it runs a `render_template` request over the Home Assistant WebSocket API, which returns only the entities the knob can control:

- Domains returned: `switch`, `light`, `cover`, `automation`, and `weather`.
- If at least one `switch`, `light`, `cover` or `automation` entity is labeled `knob` in Home Assistant, only those labeled entities are returned; `weather` entities are always included. The `knob` label filter and `label_entities()` require Home Assistant 2024.4 or newer.
- Without a `knob` label, all entities of the supported domains are returned.
- At most 200 entities are stored on the device; larger lists are truncated. The device also asks Home Assistant for at most 200 entries (`variables.limit`), so the template stops collecting once the cap is reached.
- The ESPHome API large-message batching patch is applied automatically during code generation for both the PlatformIO build system (ESPHome ≤ 2026.6.x) and the native build system (ESPHome ≥ 2026.8); no manual step is needed.

## Changes in this fork

Compared to upstream v1.0.1, this fork fixes the "Refresh Entity List" hang and crash on large Home Assistant instances plus several follow-up issues:

- The entity list is fetched with a single `render_template` request over the Home Assistant WebSocket API (server-side template keeps only the supported domains and honours the `knob` label). Responses are capped at 200 entities / 128 KB, and the `render_template` subscription is always unsubscribed afterwards.
- Registry parsing, select-option rebuild, config commit, and API-client refresh run in the main loop; the WebSocket task only accumulates bytes and hands the payload over through a mutex-protected pending buffer. Large responses no longer block the UI or exhaust memory.
- Entity names are truncated on UTF-8 character boundaries (at storage time, when building select options, and when loading persisted entities). Previously a Russian name cut mid-character produced invalid UTF-8, and Home Assistant rejected the whole `ListEntitiesSelectResponse` with `String field had bad UTF-8`, disconnecting and reconnecting in a loop.
- The ESPHome API large-message batching workaround (needed when a Select carries ~200 options) is applied automatically during code generation, so it works both on the PlatformIO build (ESPHome ≤ 2026.6.x) and on the native CMake/ESP-IDF build (ESPHome ≥ 2026.8), where `extra_scripts` no longer runs.
- Compatibility with ESPHome 2026.8+ APIs: `Select::current_option()` instead of `Select::state`, `IPAddress::str_to()` instead of `IPAddress::str()`, template-friendly select option vectors.
- Builds verified with ESPHome 2026.6.3 and 2026.10.1.

Home Assistant requirement: enable **"Allow the device to perform Home Assistant actions"** in the ESPHome integration options for this device (Settings → Devices & Services → ESPHome → *ONX2424G013 c83734* → Configure). With the default `allow_service_calls: false`, knob presses are shown on the display but Home Assistant rejects the `light.toggle` / `switch.toggle` calls.

## Requirements

Build this project with **ESPHome 2026.6.3** or newer. Both the 2026.6.x line and ESPHome 2026.8+ are supported (tested against 2026.10.1). Docker is recommended to keep ESPHome, PlatformIO, and ESP-IDF versions consistent.

- Docker, or a local Python environment with `esphome==2026.6.3`
- Network access for the first build to download dependencies
- A data-capable USB cable for serial flashing

## Clone and build

Clone the complete repository first. The configuration depends on the included `packages`, `my_components`, `assets`, and `fonts` directories.

```bash
git clone https://github.com/OpenNextion/OpenNextion-Example-ESPHome_KNOB_Control.git
cd OpenNextion-Example-ESPHome_KNOB_Control
```

### Docker (recommended)

Linux/macOS:

```bash
docker run --rm \
  -v "$PWD":/config \
  -w /config \
  ghcr.io/esphome/esphome:2026.6.3 \
  compile ONX2424G013.yaml
```

Windows PowerShell:

```powershell
docker run --rm -v "${PWD}:/config" -w /config ghcr.io/esphome/esphome:2026.6.3 compile ONX2424G013.yaml
```

### Local ESPHome

```bash
python -m pip install "esphome==2026.6.3"
esphome version
esphome compile ONX2424G013.yaml
```

After a successful build, the factory image for browser-based first-time installation is `.esphome/build/onx2424g013/.pioenvs/onx2424g013/firmware.factory.bin`. For later serial uploads, use the firmware file reported by the ESPHome build output.

## Flash over USB

Docker is the recommended way to compile and flash this project. You do not need to install ESPHome locally when flashing with Docker or ESPHome Web.

### Docker (recommended, Linux)

Pass the actual serial device through to the container. The following command compiles the project if needed and uploads it:

```bash
docker run --rm \
  --device=/dev/ttyACM0 \
  -v "$PWD":/config \
  -w /config \
  ghcr.io/esphome/esphome:2026.6.3 \
  run --device /dev/ttyACM0 ONX2424G013.yaml
```

Replace `/dev/ttyACM0` with the serial port of your device. If the device does not enter download mode automatically, hold `BOOT` (GPIO0), tap `RESET` if available, then release `BOOT` when upload starts. Restart the device after uploading completes.

### Local ESPHome CLI

If you installed `esphome==2026.6.3` locally as described above, connect the device and run the following command. It compiles the firmware when needed and uploads it. When prompted, select the corresponding serial port, for example `/dev/ttyACM0` or `/dev/ttyUSB0` on Linux, or `COM3` on Windows:

```bash
esphome run ONX2424G013.yaml
```

Specify the serial port if necessary:

```bash
esphome run --device /dev/ttyACM0 ONX2424G013.yaml
```

## Browser installation (first-time flash)

For browser flashing, open [ESPHome Web](https://web.esphome.io/) in Chrome or Edge, connect the device, and select a factory firmware file. This method requires a Chromium-based browser with Web Serial support, but does not require a local ESPHome installation.

- **Build from source:** select `.esphome/build/onx2424g013/.pioenvs/onx2424g013/firmware.factory.bin` after compiling the repository.
- **No local build:** download `onx2424g013.factory_v1.0.1.bin` from the [v1.0.1 release](https://github.com/OpenNextion/OpenNextion-Example-ESPHome_KNOB_Control/releases/tag/v1.0.1), then select that file in ESPHome Web.

## User manual

For device setup, Wi-Fi provisioning, Home Assistant integration, screen operation, and OTA updates, see the [online user manual](https://opennextion.github.io/OpenNextion-Example-ESPHome_KNOB_Control/).

## Project layout

- `ONX2424G013.yaml`: main ESPHome configuration.
- `packages/`: split device, connectivity, display, input, and feature configuration.
- `my_components/`: local ESPHome components.
- `assets/` and `fonts/`: UI assets and fonts.
- `docs/`: README demonstration media.
- `firmware/`: release OTA image and manifest.
- `User Manual/`: bundled web user manual.

## Troubleshooting

- **Serial port is not detected:** Use a data-capable USB cable, confirm that the operating system sees the serial device, and close any application using that port.
- **Assets or components cannot be found during compilation:** Run commands from the root of the cloned repository and retain all project directories.
