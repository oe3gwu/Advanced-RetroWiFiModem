# Firmware variants

| Directory | Module | Hardware | PPP + NAT | Arduino IDE board menu |
|-----------|--------|----------|-----------|------------------------|
| [`wemos-d1-mini/`](wemos-d1-mini/Advanced-RetroWiFiModem/) | Wemos D1 mini (ESP8266) | Shared [`kicad/wemos/`](../kicad/wemos/) PCB | No | **LOLIN(WEMOS) D1 R2 & mini** |
| [`wemos-c3-mini/`](wemos-c3-mini/Advanced-RetroWiFiModem/) | Wemos C3 Mini (ESP32-C3) | Same `kicad/wemos/` PCB (drop-in) | Yes | **LOLIN C3 Mini** |
| [`esp32-wroom-da/`](esp32-wroom-da/Advanced-RetroWiFiModem/) | ESP32-WROOM-DA | Bring-your-own wiring | Yes | **ESP32-WROOM-DA Module** |

**One PCB, two Wemos modules:** Order or build from `kicad/wemos/`, then install either a D1 mini or C3 Mini and flash the matching firmware. The PCB layout was originally designed for the ESP8266 D1 mini; the C3 Mini uses the same socket.

**ESP32-WROOM-DA:** Firmware only — no PCB design in this repository.

**RAW transparent mode (`AT$MODE=RAW`):** Experimental dataset-style mode on branch `transparent` for all three firmware variants (see main [README](../README.md)).

> **Do not use** *LOLIN(WEMOS) D1* (D1 **R1**) for the Wemos PCB — wrong GPIO mapping for RS-232 control lines. Use **D1 R2 & mini** only.

See the main [README](../README.md) for board package versions, GPIO pinout, flash size, and setup.
