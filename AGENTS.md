# AGENTS.md

## Project

ESP32-S2 (Lolin S2 Mini) firmware, PlatformIO + Arduino framework. All application code lives in a single file: `src/main.cpp` (~400 lines). `include/`, `lib/`, `test/` are empty scaffolding — there are no tests.

## Commands

- Build (the only verification step; no linter/typecheck/test setup exists): `pio run`
- Upload: `pio run --target upload`
- Serial monitor: `pio device monitor -b 115200` (`monitor_port` in platformio.ini is hardcoded to `/dev/cu.usbmodem01`, macOS USB CDC)

Trust `platformio.ini` over the README: the README's dependency-version block and hardware notes have drifted (e.g. `temp_offset` is `5.0` in code, README claims `-3.0`; README lists older DallasTemperature/ArduinoJson versions).

## Conventions

- **Fully non-blocking loop**: LED blinking, beeper, temperature reads, radar polling, MQTT reconnect, and heartbeat are all `millis()`-based state machines (see `loop()`). Do not add `delay()` or blocking loops; a blocking call breaks the OTA web server and sensor timing. Deferred restarts use the `restartRequested` flag via `requestRestart()` — never call `ESP.restart()` inline.
- **MQTT topics are fixed-size char arrays** declared at the top of `main.cpp`, configurable via the WiFiManager portal (prefix default `sensorcluster`). Keep buffer sizes (`char mqtt_topic[100]` etc.) when touching topic config.
- Pins and intervals are `#define`/`const` at the top of `main.cpp`; update the README pin table if hardware mapping changes.
- Comments, serial output, and README are in German — follow that style.
