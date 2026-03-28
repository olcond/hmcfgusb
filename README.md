# hmcfgusb

Linux/Unix utilities for the **HM-CFG-USB(2)** HomeMatic USB Configuration Adapter from ELV, powered by [libusb 1.0](http://www.libusb.org/).

[![Build](https://github.com/olcond/hmcfgusb/actions/workflows/build.yml/badge.svg)](https://github.com/olcond/hmcfgusb/actions/workflows/build.yml)
[![Tests](https://github.com/olcond/hmcfgusb/actions/workflows/test.yml/badge.svg)](https://github.com/olcond/hmcfgusb/actions/workflows/test.yml)
[![Docker Image CI](https://github.com/olcond/hmcfgusb/actions/workflows/docker-image.yml/badge.svg)](https://github.com/olcond/hmcfgusb/actions/workflows/docker-image.yml)
[![GitHub Release](https://img.shields.io/github/v/release/olcond/hmcfgusb)](https://github.com/olcond/hmcfgusb/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## Overview

The HM-CFG-USB allows sending and receiving [BidCoS](http://homegear.eu/index.php/BidCoS%C2%AE_Packets) packets to control [HomeMatic](http://www.homematic.com/) home automation devices — sockets, switches, sensors, and more.

**Key features:**

- `hmland` — emulates the HomeMatic LAN adapter protocol for use with [Fhem](http://fhem.de/), [Homegear](https://www.homegear.eu/), and the CCU / eQ-3 configuration software
- `hmsniff` — sniffs and displays raw BidCoS packets over the air
- `flash-hmcfgusb` — flashes firmware to the HM-CFG-USB stick
- `flash-ota` — over-the-air firmware updates for HomeMatic devices (supports HM-CFG-USB, CUL/COC, HM-MOD-UART)
- AES-signed device support (e.g. [KeyMatic](http://www.elv.de/homematic-funk-tuerschlossantrieb-keymatic-silber-inkl-funk-handsender.html))

---

## Table of Contents

- [Installation](#installation)
- [Quick Start — hmland](#quick-start--hmland)
- [Docker](#docker)
- [Flashing HM-CFG-USB Firmware](#flashing-hm-cfg-usb-firmware)
- [Over-the-Air Device Updates](#over-the-air-device-updates)
- [Compatibility Notes](#compatibility-notes)
- [Security Notes](#security-notes)
- [Credits](#credits)

---

## Installation

### Prerequisites

```bash
apt-get install libusb-1.0-0-dev build-essential clang git
```

### Get the source

**Latest release** (recommended for production):

```bash
# Download from https://github.com/olcond/hmcfgusb/releases/latest
tar xzf hmcfgusb-<version>.tar.gz
cd hmcfgusb-<version>
```

**Development version** via git:

```bash
git clone https://github.com/olcond/hmcfgusb
cd hmcfgusb
```

**Development snapshot** (no git required):

```bash
curl -L https://github.com/olcond/hmcfgusb/archive/refs/heads/main.tar.gz | tar xz
cd hmcfgusb-main
```

### Build

```bash
make
# or, if clang is not available:
CC=gcc make
```

### udev rules (optional, recommended)

Allow non-root users to access the USB adapter:

```bash
sudo cp hmcfgusb.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```

---

## Quick Start — hmland

`hmland` emulates a HomeMatic LAN adapter over TCP so that home automation software can use the HM-CFG-USB directly.

1. Plug in the HM-CFG-USB adapter.
2. Start hmland (verbose mode for first run):

   ```bash
   ./hmland -p 1234 -D
   ```

3. Configure Fhem:

   ```
   define hmusb HMLAN 127.0.0.1:1234
   attr hmusb hmId <hmId>
   ```

**Common options:**

| Flag | Description |
|------|-------------|
| `-p <port>` | TCP listen port (default: 1000) |
| `-D` | Debug + verbose output |
| `-d` | Run as daemon (with PID file) |
| `-l <file>` | Log to file |
| `-I` | Impersonate HM-LAN-IF (see [Compatibility Notes](#compatibility-notes)) |

---

## Docker

A pre-built multi-arch image (`amd64`, `arm64`, `arm/v7`) is available from the GitHub Container Registry:

```bash
docker pull ghcr.io/olcond/hmcfgusb
docker run -p 1234:1234 --device /dev/bus/usb ghcr.io/olcond/hmcfgusb
```

The container runs `hmland -v -p 1234 -I` by default and exposes port `1234`.

---

## Flashing HM-CFG-USB Firmware

1. Build the utilities (see [Installation](#installation)).
2. Download the firmware update from [eQ-3](http://www.eq-3.de/downloads.html) and extract the `.enc` file.
3. Make sure `hmland` is **not** running.
4. Flash the firmware:

   ```bash
   ./flash-hmcfgusb hmusbif.XXXX.enc
   # use sudo if permission is denied
   ```

---

## Over-the-Air Device Updates

Supports **HM-CFG-USB(2)**, [culfw](http://culfw.de/culfw.html)/[a-culfw](https://forum.fhem.de/index.php?topic=35064.0)/[tsculfw](https://forum.fhem.de/index.php?topic=24436.0)-based devices (CUL, COC, …), and **HM-MOD-UART**.

1. Build the utilities (see [Installation](#installation)).
2. Download the new device firmware from [eQ-3](http://www.eq-3.de/downloads.html) and extract the `.eq3` file.
3. Make sure `hmland` is **not** running.
4. Flash with the appropriate command for your hardware:

   ```bash
   # HM-CFG-USB(2)
   ./flash-ota -f firmware.eq3 -s KEQ0123456

   # CUL / COC (culfw-based serial device)
   ./flash-ota -f firmware.eq3 -s KEQ0123456 -c /dev/ttyACM0

   # HM-MOD-UART
   ./flash-ota -f firmware.eq3 -s KEQ0123456 -U /dev/ttyAMA0
   ```

**Automatic bootloader entry** (no manual button press required):

```bash
./flash-ota -f firmware.eq3 -C ABCDEF -D 012345 -K 01:00112233445566778899AABBCCDDEEFF
```

`-C` = HMID of your central, `-D` = HMID of the target device, `-K` = AES key index and key (only needed if AES signing is active on the device).

---

## Compatibility Notes

**Older Fhem / Homegear versions:**
Fhem before 2015-06-19 and Homegear before 2015-07-01 require the `-I` flag when connecting to `hmland`. This makes hmland identify itself as `HM-LAN-IF` instead of `HM-USB-IF`. Without it, older software will not maintain a stable connection. Modern versions of Fhem, Homegear, and eQ-3 rfd work correctly without `-I`.

---

## Security Notes

> [!WARNING]
> Versions **before 0.101** do not correctly transmit the AES channel-mask to the HM-CFG-USB. This causes signature requests to not be generated by the device in most cases, allowing unsigned messages to be processed by host software. If you rely on authenticated messages (e.g. `aesCommReq` in Fhem) from door sensors, remotes, or similar devices, upgrade to **≥ 0.101**.

---

## Credits

- Original author: **Michael Gernoth** &lt;michael@gernoth.net&gt; (2013–2020)
- AES implementation: [Brad Conte](https://github.com/B-Con/crypto-algorithms) (public domain)
- [HM-CFG-USB(2) product page](http://www.eq-3.de/Downloads/eq3/downloads_produktkatalog/homematic/bda/HM-CFG-USB-2_-UM-eQ-3-150129-web.pdf) — [ELV](http://www.elv.de/)
