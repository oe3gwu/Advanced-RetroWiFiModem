# Advanced Retro WiFi Modem

> **AI-assisted reimplementation — not for production use**  
> This repository is an AI-assisted reimplementation and extension of the original [Retro WiFi Modem](https://github.com/oe3gwu/RetroWiFiModem). New features (DFU, PPP, RAW transparent mode) are experimental. Do not use in production or safety-critical environments without your own review, testing, and hardening.

An RS-232 WiFi modem with Hayes AT commands, status LEDs, and a full set of RS-232 control lines.

This repository offers two paths:

| Variant | Scope |
|---------|-------|
| **ESP8266** | Turnkey solution — firmware, KiCad project, Gerbers, and BOM |
| **LOLIN C3 Mini (ESP32-C3)** | Drop-in replacement for the Wemos D1 mini on the ESP8266 PCB — firmware only; **currently under test** |
| **ESP32-WROOM-DA** | Firmware; KiCad PCB in progress (`kicad/esp32/`) — **not verified, do not build yet** |

## Feature overview

| Feature | ESP8266 | ESP32-C3 | ESP32-WROOM-DA | Maturity |
|---------|---------|----------|----------------|----------|
| Hayes AT, `ATDT`, Telnet, TCP server, NVRAM | ✓ | ✓ | ✓ | Stable (core functionality) |
| OTA via Arduino IDE (developer) | ✓ | ✓ | ✓ | Stable |
| **DFU** (`AT$DFU=…`) | ✓ | ✓ | ✓ | **Experimental** — see below |
| **PPP + NAT** (`ATD*99#`) | ✗ (stub) | ✓ | ✓ | ESP32: tested with Linux `pppd`; **C3: under test** |
| **RAW / transparent mode** (`AT$MODE=RAW`) | ✗ | ✓ | ✓ | **Experimental** — see [RAW mode](#raw--transparent-mode--experimental) |
| **PCB (KiCad / Gerbers)** | ✓ | ✓ (same ESP8266 PCB) | ✗ | ESP8266: production-ready; ESP32-WROOM layout **work in progress, untested** |

## What is in this repository

### ESP8266 (turnkey)

| Path | Contents |
|------|----------|
| `firmware/esp8266/Advanced-RetroWiFiModem/` | Arduino sketch for Wemos D1 mini |
| `kicad/esp8266/` | KiCad project (schematic, layout, libraries) |
| `kicad/esp8266/gerbers/` | Ready-to-order Gerber files |

Order the PCB, solder the parts, plug in a Wemos D1 mini, flash the firmware — done.

### LOLIN C3 Mini (ESP32-C3) — drop-in on ESP8266 PCB

| Path | Contents |
|------|----------|
| `firmware/esp32-c3/Advanced-RetroWiFiModem/` | Arduino sketch for [LOLIN C3 Mini](https://www.wemos.cc/en/latest/c3/c3_mini.html) |

> **Drop-in replacement — currently under test**  
> The LOLIN C3 Mini uses the same **Wemos D1 mini footprint** as the ESP8266 board. On the RetroWiFiModem ESP8266 PCB you can swap the Wemos D1 mini for a C3 Mini, flash `firmware/esp32-c3/`, and gain **PPP + NAT** (ESP8266 is too small for a reliable PPP stack). **This combination is being validated right now** — treat pinout and behaviour as correct in theory, not yet fully confirmed in the field.

**Why it fits mechanically and electrically**

The KiCad layout (`kicad/esp8266/`) powers the module from the D1 mini header like this:

| D1 mini pad | ESP8266 PCB | LOLIN C3 Mini | Notes |
|-------------|-------------|---------------|-------|
| **1** | **+5 V** | **VBUS / 5 V** | Module supply from the modem's 5 V rail |
| **2** | **GND** | **GND** | Common ground |
| **16** | 3.3 V | 3V3 | **Not connected** on the PCB (modem has its own 3.3 V regulator) |

So for this PCB only **5 V and GND** matter on the module header — and those match the C3 Mini pinout. No board rework required. Modem signal pins use the same **D-pin positions** on the shield; see [GPIO pinout comparison](#gpio-pinout-comparison).

**Caveats (under test)**

- **GPIO7** (DTR) is also the onboard RGB LED on the C3 Mini.
- **GPIO8** (DSR) is an ESP32-C3 boot strap pin — fine as an output after boot; avoid pulling it low during reset.
- After swapping from ESP8266, run **`AT&F`** once (EEPROM magic `0x4323`, default mDNS `esp32c3modem`).
- **Arduino IDE:** board package [esp32 by Espressif](https://docs.espressif.com/projects/arduino-esp32/) **3.x**, board *LOLIN C3 Mini*. PPP/NAT works with the stock lwIP build (no extra `build_opt.h` hacks).
- With default **USB CDC on boot**, RS-232 uses UART0 via `serial_port.h` (`Serial0` on GPIO20/21); USB remains available for upload/debug.

### ESP32-WROOM-DA (firmware + PCB)

| Path | Contents |
|------|----------|
| `firmware/esp32/Advanced-RetroWiFiModem/` | Arduino sketch port for ESP32-WROOM-DA |
| `kicad/esp32/` | KiCad project (schematic, layout, Gerbers) — same board outline as ESP8266, ESP32 dev board instead of Wemos D1 mini |

> **ESP32 PCB — work in progress**  
> The ESP32 KiCad layout in `kicad/esp32/` is still under development. It has **not** been built or tested on real hardware and is **not expected to work** as shipped. Use the ESP8266 turnkey PCB for a known-good board; treat the ESP32 files as a draft for contributors only.

GPIO mapping for each firmware variant is in [GPIO pinout comparison](#gpio-pinout-comparison).

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

- **PPP dial-up with NAT (ESP32 only):** `ATD*99#` or `AT$PPP=1` — retro host gets IP `192.168.240.2`, modem `192.168.240.1`, Internet access via WiFi NAT. Works on **ESP32-C3** (LOLIN C3 Mini drop-in) and **ESP32-WROOM-DA**; not on ESP8266.
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

### GPIO pinout comparison

Modem signals on the **ESP8266 turnkey PCB** use fixed **D-pin positions** on the Wemos D1 mini header. The LOLIN C3 Mini maps different GPIO numbers to those same positions (drop-in, **under test**). The ESP32-WROOM-DA draft PCB uses a different 30-pin dev board — column below reflects **intended** wiring, not validated hardware.

| Signal | D-pin | ESP8266 GPIO | C3 GPIO | WROOM GPIO | Connection |
|--------|-------|--------------|---------|------------|------------|
| Serial TX | Tx | 1 | 21 | TX0 | MAX3237 (via OR gate) |
| Serial RX | Rx | 3 | 20 | RX0 | MAX3237 |
| DSR | D2 | 4 | 8 | 4 | MAX3237 |
| DCD | D1 | 5 | 10 | 5 | MAX3237 |
| DTR (input) | D3 | 0 | 7 | 34 | MAX3237 |
| TXEN | D5 | 14 | 2 | 14 | OR gate (mask boot garbage) |
| RI | D6 | 12 | 3 | 12 | MAX3237 + LED |
| RTS (input) | D7 | 13 | 4 | 13 | MAX3237 |
| CTS (output) | D8 | 15 | 5 | 15 | MAX3237 |

Firmware: `firmware/esp8266/…/Advanced-RetroWiFiModem.h`, `firmware/esp32-c3/…/Advanced-RetroWiFiModem.h`, `firmware/esp32/…/Advanced-RetroWiFiModem.h`

> RTS/CTS are named from the modem (DCE) perspective. On ESP32-WROOM-DA, do not use GPIO0 for DTR — it is unavailable on 30-pin headers and pulling it affects boot mode.

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
| `ppp.h` | PPP dial-up + NAT (ESP32 / ESP32-C3; ESP8266 stub) |

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

### LOLIN C3 Mini (ESP32-C3) — `firmware/esp32-c3/Advanced-RetroWiFiModem/`

Drop-in replacement for the **Wemos D1 mini** on the ESP8266 turnkey PCB — see [LOLIN C3 Mini](#lolin-c3-mini-esp32-c3--drop-in-on-esp8266-pcb) and [GPIO pinout comparison](#gpio-pinout-comparison). **Currently under test.**

**Arduino IDE — requirements:**

1. Install the [esp32 by Espressif](https://docs.espressif.com/projects/arduino-esp32/) board package **3.x** (PPP/NAT needs the current lwIP build)
2. Board: *LOLIN C3 Mini*
3. USB CDC On Boot: *Enabled* (default) — RS-232 still uses UART0 on GPIO20/21 via `serial_port.h`

Open the sketch folder in the Arduino IDE, compile, and flash. After swapping from an ESP8266 module, run **`AT&F`** once.

### ESP32-WROOM-DA — `firmware/esp32/Advanced-RetroWiFiModem/`

For the ESP32 PCB in `kicad/esp32/` (30-pin ESP32-WROOM-DA dev board with USB-C, replacing the Wemos D1 mini; the DFPlayer audio section is omitted on this board).

> **ESP32 PCB — not ready for use**  
> Do **not** order or assemble the ESP32 Gerbers yet. The layout is still in development, has **not** been validated on real hardware, and **does not work** as a modem board today. The ESP32 **firmware** can be used on your own hardware; the repository PCB is for ongoing layout work only.

Pin mapping — see [GPIO pinout comparison](#gpio-pinout-comparison). For custom wiring, adjust the `#define` lines for CTS, RTS, RI, DSR, DCD, DTR, and TXEN in `Advanced-RetroWiFiModem.h`.

**Arduino IDE — requirements:**

1. Install the [esp32 by Espressif](https://docs.espressif.com/projects/arduino-esp32/) board package
2. Board: *ESP32-WROOM-DA Module*

Open the sketch folder in the Arduino IDE, compile, and flash.

> The ESP8266 PCB is **not** compatible with an ESP32-WROOM-DA module (different module, different boot strapping on GPIO 12 and 15). A **LOLIN C3 Mini** fits the same D1 mini socket on the ESP8266 PCB.

> EEPROM magic number: ESP8266 `0x4321`, ESP32-C3 `0x4323`, ESP32-WROOM-DA `0x4322` — settings are not interchangeable between platforms.

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
| `AT$MDNS=name` | mDNS name (default: `espmodem` / `esp32c3modem` / `esp32modem`) |
| `AT&Z0=host:port,alias` | Store speed dial |

Full help on device: `AT?`

## AT commands (quick reference)

Multiple commands per line are supported (`AT S0=1 Q0 V1`). String arguments (`AT$SSID=` etc.) must be at the end of the line.

**Connection:** `ATDT[+=-]host[:port]`, `ATDSn`, `ATA`, `ATH`, `ATO`, `+++` (escape)

**WiFi:** `ATC0`/`ATC1`, `ATI`, `ATGEThttp://…`, `ATRD`/`ATRT` (UTC via NTP, `pool.ntp.org`)

**Configuration:** `AT&W`, `AT&F`, `AT&V0`/`AT&V1`, `AT&Zn=…`, `AT$SSID=`, `AT$PASS=`, `AT$AE=`, `AT$BM=`, `AT&R=`, `ATZ`

**Behaviour:** `ATE0`/`ATE1`, `ATQ0`/`ATQ1`, `ATV0`/`ATV1`, `ATX0`/`ATX1`, `ATS0=n`, `ATS2=n`, `AT&D0`–`AT&D3`

**Experimental (AI reimplementation):** `AT$DFU=…`, `ATD*99#`, `AT$PPP=1`/`AT$PPP=0`, `AT$MODE=RAW` — details in sections below

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

## RAW / transparent mode — **experimental**

> **Disclaimer / use at your own risk**  
> RAW mode is an **experimental** dataset-style operating mode for vintage hosts that expect a dumb modem (no Hayes commands on the serial line). DTR timing, RS-232 polarity, and host behaviour vary widely — test with **your** hardware. **The author accepts no liability** for misconfiguration or failed connections.

RAW mode is a **persistent** operating mode stored in NVRAM (`AT$MODE=RAW` / `AT$MODE=AT`). After reboot the modem stays in the configured mode until changed again.

| | **AT mode** (default) | **RAW mode** (experimental) |
|--|----------------------|----------------------------|
| Serial commands | Hayes `AT…` | None (byte pipe only) |
| Outbound dial | `ATDT…` | DTR asserted → Speed-Dial slot 0 |
| PPP | `ATD*99#` | Not available |
| Connect text | `CONNECT` / `NO CARRIER` | DCD only (no text) |
| Boot banner | WiFi status + `Mode: AT …` | WiFi status + `Mode: RAW experimental …` |

### Setup (once, in AT mode)

```
AT&Z0=192.168.1.10:4000,myhost
AT$SB=110
AT$MODE=RAW
ATZ
```

Speed-Dial slot 0 defines the TCP target when the DTE asserts DTR. Recommended: 110 or 300 baud, `ATNET0` if you switch back to Hayes for BBS use.

### Return to AT mode (manual — not automatic)

1. Drop any active connection (release DTR).
2. Hold **DTR inactive for 5 seconds** — a **120 second maintenance window** opens and Hayes commands are accepted.
3. Type **`AT$MODE=AT`** — saved to NVRAM immediately.
4. **`ATZ`** — reboot into Hayes mode (recommended).

If step 3 is not completed within 120 seconds, the window closes and the modem **remains in RAW mode** (repeat the gesture).

### FAQ

**Does the modem switch to AT automatically after 5 seconds?**  
No. Five seconds of inactive DTR only opens the maintenance window. You must type `AT$MODE=AT` yourself.

**What if the 120 second window expires?**  
The modem stays in RAW mode. Repeat: disconnect, hold DTR inactive 5 s, then `AT$MODE=AT`.

## PPP (dial-up IP) — ESP32 / ESP32-C3

PPP turns the serial line into an IP link — for retro systems with `pppd`, Windows dial-up, or a native PPP stack. **Implemented on ESP32 platforms** (lwIP `pppos` + NAT). On **ESP8266**, `ATD*99#` returns `NO CARRIER` (no PPP stack in the Arduino core). **LOLIN C3 Mini drop-in: under test.**

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
- **ESP8266 / PPP:** Not available — `ATD*99#` returns `NO CARRIER`. Use **ESP32-C3** (LOLIN C3 Mini drop-in) or ESP32-WROOM-DA for dial-up IP.
- **ESP32-C3 / LOLIN C3 Mini:** Drop-in on ESP8266 PCB — **under test**; PPP/NAT expected to work with esp32 core 3.x.
- **DFU:** Experimental; wrong images can brick the device. No HTTPS. HTTP DFU requires WiFi; XMODEM does not.
- **PPP / ESP32:** Primarily tested with Linux `pppd`; Windows DUN and CHAP still open.

## License

[GNU GPL v3](LICENSE.txt). Based on Virtual Modem code by Jussi Salin (2016), with extensions by Daniel Jameson, Stardot Contributors, and Paul Rickards (2018).
