# Firmware variants

| Directory | Module | Hardware | PPP + NAT |
|-----------|--------|----------|-----------|
| [`wemos-d1-mini/`](wemos-d1-mini/Advanced-RetroWiFiModem/) | Wemos D1 mini (ESP8266) | Shared [`kicad/wemos/`](../kicad/wemos/) PCB | No |
| [`wemos-c3-mini/`](wemos-c3-mini/Advanced-RetroWiFiModem/) | Wemos C3 Mini (ESP32-C3) | Same `kicad/wemos/` PCB (drop-in) | Yes |
| [`esp32-wroom-da/`](esp32-wroom-da/Advanced-RetroWiFiModem/) | ESP32-WROOM-DA | Bring-your-own wiring | Yes |

**One PCB, two Wemos modules:** Order or build from `kicad/wemos/`, then install either a D1 mini or C3 Mini and flash the matching firmware. The PCB layout was originally designed for the ESP8266 D1 mini; the C3 Mini uses the same socket.

**ESP32-WROOM-DA:** Firmware only — no PCB design in this repository.

See the main [README](../README.md) for Arduino IDE board settings, GPIO pinout, and setup.
