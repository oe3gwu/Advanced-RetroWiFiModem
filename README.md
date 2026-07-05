# Advanced Retro WiFi Modem

> **AI-assisted reimplementation — not for production use**  
> This repository is an AI-assisted reimplementation and extension of the original [Retro WiFi Modem](https://github.com/oe3gwu/RetroWiFiModem). New features (DFU, PPP) are experimental. Do not use in production or safety-critical environments without your own review, testing, and hardening.

An RS-232 WiFi modem with Hayes AT commands, status LEDs, and a full set of RS-232 control lines.

This repository offers two paths:

| Variant | Scope |
|---------|-------|
| **ESP8266** | Turnkey solution — firmware, KiCad project, Gerbers, and BOM |
| **ESP32-WROOM-DA** | Firmware only — no PCB layout; bring your own hardware |

## Feature overview

| Feature | ESP8266 | ESP32 | Maturity |
|---------|---------|-------|----------|
| Hayes AT, `ATDT`, Telnet, TCP server, NVRAM | ✓ | ✓ | Stable (core functionality) |
| OTA via Arduino IDE (developer) | ✓ | ✓ | Stable |
| **DFU** (`AT$DFU=…`) | ✓ | ✓ | **Experimental** — see below |
| **PPP + NAT** (`ATD*99#`) | ✗ (stub) | ✓ | ESP32: tested with Linux `pppd` |

## What is in this repository

### ESP8266 (turnkey)

| Path | Contents |
|------|----------|
| `firmware/esp8266/Advanced-RetroWiFiModem/` | Arduino sketch for Wemos D1 mini |
| `kicad/esp8266/` | KiCad project (schematic, layout, libraries) |
| `kicad/esp8266/gerbers/` | Ready-to-order Gerber files |

Order the PCB, solder the parts, plug in a Wemos D1 mini, flash the firmware — done.

### ESP32-WROOM-DA (firmware only)

| Path | Contents |
|------|----------|
| `firmware/esp32/Advanced-RetroWiFiModem/` | Arduino sketch port for ESP32-WROOM-DA |

No schematic, layout, or Gerbers. GPIO mapping in `firmware/esp32/Advanced-RetroWiFiModem/Advanced-RetroWiFiModem.h` follows the ESP8266 PCB and must be adapted to your wiring.

### General

| Path | Contents |
|------|----------|
| `LICENSE.txt` | GNU GPL v3 |

## Features

### Core (stable)

- RS-232 interface (DE-9) with TxD, RxD, RTS, CTS, DSR, DTR, DCD, and RI
- Hayes AT command set in WiFi232 style
- TCP connections to BBSes, Telnet servers, and other services
- Telnet protocol: real, fake (for certain BBSes), or disabled
- 10 speed-dial slots with alias names
- TCP server mode with optional password
- OTA firmware update over WiFi (Arduino IDE, developer workflow)

### New in this AI reimplementation

- **PPP dial-up with NAT (ESP32 only):** `ATD*99#` or `AT$PPP=1` — retro host gets IP `192.168.240.2`, modem `192.168.240.1`, Internet access via WiFi NAT
- **DFU (experimental, both platforms):** `AT$DFU=http://host/firmware.bin` or `AT$DFU=xmodem` — end-user update without the Arduino IDE; **at your own risk**, see [DFU](#dfu-firmware-update-via-at-command--experimental)

## ESP8266 — hardware and assembly

The PCB in `kicad/esp8266/` is designed for a [Wemos D1 mini](https://docs.wemos.cc/en/latest/d1/d1_mini.html).

| Component | Function |
|-----------|----------|
| Wemos D1 mini | ESP8266 with WiFi |
| MAX3237 | RS-232 level shifter (3.3 V ↔ ±12 V) |
| 74HCT245 | LED drivers for status indicators |
| 74HC32 | OR gate — masks boot output on the serial line |
| LM2931 | Separate 3.3 V regulator for peripherals |
| DFPlayer Mini | Provided on the PCB (not driven by firmware) |

**Power:** 5 V, centre positive, 2.1 × 5.5 mm barrel jack.

**Order PCB:** Gerbers in `kicad/esp8266/gerbers/`.

**Edit schematic:** open `kicad/esp8266/RetroWiFiModem.kicad_pro` in KiCad.

### ESP8266 pinout (Wemos D1 mini on PCB)

Defined in `firmware/esp8266/Advanced-RetroWiFiModem/Advanced-RetroWiFiModem.h`:

| Signal | GPIO | D1 mini pin | Connection |
|--------|------|-------------|------------|
| Serial TX | 1 | Tx | MAX3237 (via OR gate) |
| Serial RX | 3 | Rx | MAX3237 |
| DSR | 4 | D2 | MAX3237 |
| DCD | 5 | D1 | MAX3237 |
| DTR (input) | 0 | D3 | MAX3237 |
| TXEN | 14 | D5 | OR gate (mask boot garbage) |
| RI | 12 | D6 | MAX3237 + LED |
| RTS (input) | 13 | D7 | MAX3237 |
| CTS (output) | 15 | D8 | MAX3237 |

> RTS/CTS are named from the modem (DCE) perspective.

## Firmware

Both variants share the same module structure:

| File | Function |
|------|----------|
| `Advanced-RetroWiFiModem.ino` | Main loop, setup |
| `Advanced-RetroWiFiModem.h` | Constants, pin definitions |
| `globals.h` | Global variables, settings structure |
| `support.h` | Helpers, Telnet, connection logic |
| `at_basic.h` | Standard AT commands |
| `at_extended.h` | Extended AT commands (&D, &F, &K, &W, …) |
| `at_proprietary.h` | Proprietary AT commands (AT$…) |
| `dfu.h` / `xmodem.h` | Experimental firmware update (AT$DFU) |
| `ppp.h` | PPP dial-up + NAT (ESP32; ESP8266 stub) |

### ESP8266 — `firmware/esp8266/Advanced-RetroWiFiModem/`

For the turnkey PCB with Wemos D1 mini. In the Arduino IDE, open the sketch folder `Advanced-RetroWiFiModem.ino`.

**Arduino IDE — requirements:**

1. Board: *LOLIN(WEMOS) D1 R2 & mini*
2. ESP8266 core **3.1.2** or newer (`https://arduino.esp8266.com/stable/package_esp8266com_index.json`)
3. [ESP_EEPROM](https://github.com/jwrw/ESP_EEPROM) library, current version (2.2.0+)
4. `eeprom_storage.h` — lazy EEPROM init in `setup()` (no separate install needed)

The firmware initializes EEPROM in `setup()` with the correct flash sector (`EEPROM_start`). This makes `AT&W` work with ESP_EEPROM 2.2.x and current ESP8266 core — pinning to the older 2.1.2 release is no longer required.

**Arduino IDE:** Tools → Flash Size must match your hardware (e.g. *4MB (FS:2MB OTA:~1019KB)*).

Select board and port, compile, and flash.

### ESP32-WROOM-DA — `firmware/esp32/Advanced-RetroWiFiModem/`

For the ESP32 PCB in `kicad/esp32/` (30-pin ESP32-WROOM-DA dev board with USB-C, replacing the Wemos D1 mini; the DFPlayer audio section is omitted on this board).

The default pin mapping in `firmware/esp32/Advanced-RetroWiFiModem/Advanced-RetroWiFiModem.h` matches the ESP8266 PCB (see table above) with one exception: DTR uses **GPIO34** instead of GPIO0, because GPIO0 is not brought out on 30-pin dev boards (it is tied to the BOOT button). GPIO34 is input-only, which suits DTR. For different wiring, adjust the `#define` lines for CTS, RTS, RI, DSR, DCD, DTR, and TXEN.

| Signal | GPIO | Dev board pin | Connection |
|--------|------|---------------|------------|
| Serial TX | 1 | TX0 | MAX3237 (via OR gate) |
| Serial RX | 3 | RX0 | MAX3237 |
| DSR | 4 | D4 | MAX3237 |
| DCD | 5 | D5 | MAX3237 |
| DTR (input) | 34 | D34 | MAX3237 |
| TXEN | 14 | D14 | OR gate (mask boot garbage) |
| RI | 12 | D12 | MAX3237 + LED |
| RTS (input) | 13 | D13 | MAX3237 |
| CTS (output) | 15 | D15 | MAX3237 |

**Arduino IDE — requirements:**

1. Install the [esp32 by Espressif](https://docs.espressif.com/projects/arduino-esp32/) board package
2. Board: *ESP32-WROOM-DA Module*

Open the sketch folder in the Arduino IDE, compile, and flash.

> The ESP8266 PCB is **not** compatible with an ESP32-WROOM-DA module (different module, different boot strapping on GPIO 12 and 15).

> EEPROM magic number: ESP8266 `0x4321`, ESP32-WROOM-DA `0x4322` — settings are not interchangeable between platforms.

## Initial setup

Applies to both firmware variants. Factory default: **1200 baud, 8N1**. For initial setup, use `AT$SB=9600` followed by `AT&W`.

```
AT$SSID=MyWiFi
AT$PASS=MyPassword
ATC1
AT&W
```

Connect:

```
ATDTparticles                 ; speed dial by alias (factory default in &F)
ATDTaltair.virtualaltair.com  ; hostname
ATDT192.168.1.10:6400         ; IP with port
```

| Command | Description |
|---------|-------------|
| `AT$SB=n` | Baud rate (110 … 115200) |
| `AT$SU=dps` | Data bits, parity, stop bits (e.g. `8N1`) |
| `ATNETn` | Telnet: 0=off, 1=real, 2=fake |
| `AT&K1` | Hardware flow control (RTS/CTS) |
| `AT&Dn` | DTR behaviour: 0=ignore, 1=go offline, 2=hang up, 3=reset |
| `AT$SP=n` | TCP server port for incoming connections |
| `AT$MDNS=name` | mDNS name (default: `espmodem` / `esp32modem`) |
| `AT&Z0=host:port,alias` | Store speed dial |

Full help on device: `AT?`

## AT commands (quick reference)

Multiple commands per line are supported (`AT S0=1 Q0 V1`). String arguments (`AT$SSID=` etc.) must be at the end of the line.

**Connection:** `ATDT[+=-]host[:port]`, `ATDSn`, `ATA`, `ATH`, `ATO`, `+++` (escape)

**WiFi:** `ATC0`/`ATC1`, `ATI`, `ATGEThttp://…`, `ATRD`/`ATRT` (UTC via NTP, `pool.ntp.org`)

**Configuration:** `AT&W`, `AT&F`, `AT&V0`/`AT&V1`, `AT&Zn=…`, `AT$SSID=`, `AT$PASS=`, `AT$AE=`, `AT$BM=`, `AT&R=`, `ATZ`

**Behaviour:** `ATE0`/`ATE1`, `ATQ0`/`ATQ1`, `ATV0`/`ATV1`, `ATX0`/`ATX1`, `ATS0=n`, `ATS2=n`, `AT&D0`–`AT&D3`

**Experimental (AI reimplementation):** `AT$DFU=…`, `ATD*99#`, `AT$PPP=1`/`AT$PPP=0` — details in sections below

## DFU (firmware update via AT command) — **experimental**

> **Disclaimer / use at your own risk**  
> DFU is an **experimental** feature with no warranty. A wrong binary (wrong platform, corrupted file, interruption during flashing) can **brick** the device. Use only the **correct** `.bin` for **your** platform (ESP8266 or ESP32). Document a backup of settings (`AT&V1`) before updating. Recovery from a brick usually requires a **serial re-flash** over USB/UART (ESP8266: internal Wemos pins; ESP32: depends on your hardware). **The author accepts no liability** for damage from DFU use.

DFU does not replace proven **OTA via the Arduino IDE** (see [OTA updates](#ota-updates)). DFU targets end users without a development environment — with the risk noted above.

**Requirements:** command mode (not online), no active TCP session. HTTP DFU additionally requires WiFi connected (`ATC1`).

### HTTP DFU

```
AT$DFU=http://192.168.1.10/Advanced-RetroWiFiModem.ino.bin
```

The modem downloads the file over HTTP (no TLS), verifies it, and writes to the OTA partition. Progress on the serial console: `DOWNLOADING`, `VERIFYING`, `FLASHING`, `REBOOTING`.

```mermaid
sequenceDiagram
    participant User as Terminal
    participant Modem as Advanced-RetroWiFiModem
    participant HTTP as HTTP-Server
  User->>Modem: AT$DFU=http://host/fw.bin
  Note over Modem: WARNING: EXPERIMENTAL DFU
  Modem->>HTTP: GET /fw.bin
  HTTP-->>Modem: Firmware-Binary
  Modem-->>User: DOWNLOADING n%
  Modem-->>User: VERIFYING
  Modem-->>User: FLASHING
  Modem-->>User: REBOOTING
  Modem->>Modem: Reboot with new firmware
```

### XMODEM DFU (no HTTP server)

```
AT$DFU=xmodem
```

Then send the matching `.bin` file from your terminal program via XMODEM.

### Status

```
AT$DFU?
```

Possible responses: `idle`, `downloading`, `verifying`, `flashing`, `xmodem`, `error …`

## PPP (dial-up IP) — ESP32

PPP turns the serial line into an IP link — for retro systems with `pppd`, Windows dial-up, or a native PPP stack. **Implemented on ESP32 only** (lwIP `pppos` + NAT). On **ESP8266**, `ATD*99#` returns `NO CARRIER` (no PPP stack in the Arduino core).

| Parameter | Value |
|-----------|-------|
| Modem IP (local) | `192.168.240.1` |
| Peer IP (retro host) | `192.168.240.2` |
| Authentication | PAP (empty user/password); CHAP only if present in lwIP build |
| Recommended baud rate | `AT$SB=57600` and `AT&W` before testing |

**Dial in:**

```
ATD*99#
```

or `AT$PPP=1` — then start `pppd` or Windows dial-up on the retro host.

**Hang up:** `ATH` or `AT$PPP=0`

`ATDT` (TCP) and PPP are mutually exclusive — only one online session at a time.

```mermaid
flowchart LR
  subgraph retro [Retro host]
    PPPD[pppd / DUN]
  end
  subgraph modem [ESP32 Modem]
    SER[RS-232]
    PPP[PPP stack]
    NAT[NAT]
    WIFI[WiFi STA]
  end
  subgraph lan [Internet]
    NET[Target host]
  end
  PPPD <-->|PPP frames| SER
  SER <--> PPP
  PPP <--> NAT
  NAT <--> WIFI
  WIFI --> NET
```

### Linux example (`pppd`)

Adjust serial port and baud rate. Important: `local nocrtscts` (no hardware handshake on the PC), otherwise DTR/GPIO0 can reset the ESP32.

```bash
sudo pppd /dev/ttyUSB1 57600 local nocrtscts nodetach noauth \
  defaultroute replacedefaultroute \
  user "" password "" \
  192.168.240.2:192.168.240.1 \
  connect 'chat -v -t10 ABORT "NO CARRIER" ABORT "ERROR" "" "AT\r" "OK" "ATD*99#\r" "CONNECT"'
```

After `CONNECT` and successful IPCP:

```bash
ping 192.168.240.1          # modem
ping -I ppp0 8.8.8.8        # Internet via NAT (bring LAN interface down if needed)
```

> **Note:** If a default route over Ethernet already exists, `defaultroute` alone is often not enough — use `replacedefaultroute` or manually `ip route replace default dev ppp0`.

**Still lightly tested:** Windows 9x/98 DUN, Amiga/DOS native PPP, TCP applications over NAT (mostly ICMP/ping so far), long-running operation.

## OTA updates

With an active WiFi connection: Arduino IDE → Sketch → Upload Using Network Address.

> After a firmware update with new EEPROM fields (e.g. `dtrHandling`), run `AT&W` or `AT&F` once so the NVRAM structure matches.

## Known limitations

- **Baud rate:** No auto-detection — `AT$SB` must match your terminal.
- **Linux Telnet / binary files:** Many `0xFF` bytes through `telnetd` can drop the connection (daemon issue, not the modem). Xmodem/Ymodem with 128-byte blocks as a workaround.
- **ESP8266 / RTS/CTS:** With `AT&K1` and long RTS idle, the watchdog may trigger. The firmware patches the UART send loop with `yield()`.
- **ESP8266 / PPP:** Not available — `ATD*99#` returns `NO CARRIER`. Use ESP32 for dial-up IP.
- **DFU:** Experimental; wrong images can brick the device. No HTTPS. HTTP DFU requires WiFi; XMODEM does not.
- **PPP / ESP32:** Primarily tested with Linux `pppd`; Windows DUN and CHAP still open.

## License

[GNU GPL v3](LICENSE.txt). Based on Virtual Modem code by Jussi Salin (2016), with extensions by Daniel Jameson, Stardot Contributors, and Paul Rickards (2018).
