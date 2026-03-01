# CLAUDE.md — AI Assistant Guide for hmcfgusb

## Project Overview

**hmcfgusb** (version `0.103-git`) is a collection of C utilities for using the HM-CFG-USB(2) (HomeMatic USB Configuration Adapter) from ELV on Linux/Unix systems via libusb 1.0.

The project enables:
- Sending and receiving BidCoS packets to control HomeMatic home automation devices
- Emulating the HomeMatic LAN configuration adapter protocol (for use with Fhem, Homegear, CCU)
- Flashing firmware to HomeMatic devices (HM-CFG-USB, CUL-based devices, HM-MOD-UART)
- Monitoring and sniffing HomeMatic wireless traffic
- Supporting AES-signed devices (e.g. KeyMatic)

**License:** MIT
**Original Author:** Michael Gernoth <michael@gernoth.net> (2013–2020)

---

## Repository Structure

```
hmcfgusb/
├── Source files
│   ├── hmcfgusb.c / .h       # libusb driver wrapper for HM-CFG-USB device
│   ├── hmland.c               # HM-CFG-LAN protocol emulation daemon (main binary)
│   ├── hmsniff.c              # HomeMatic wireless packet sniffer
│   ├── flash-hmcfgusb.c       # Firmware flasher for HM-CFG-USB device
│   ├── flash-hmmoduart.c      # Firmware flasher for HM-MOD-UART device
│   ├── flash-ota.c            # Over-the-air firmware updater (multi-device)
│   ├── hmuartlgw.c / .h       # HM-MOD-UART / HM-LGW serial driver
│   ├── culfw.c / .h           # culfw serial protocol driver (CUL, COC, etc.)
│   ├── hm.c / .h              # HomeMatic packet format and AES signing
│   ├── aes.c / .h             # AES-128/192/256 implementation (Brad Conte, public domain)
│   ├── firmware.c / .h        # Intel HEX firmware file parser + CRC16
│   ├── util.c / .h            # ASCII/nibble helpers
│   └── hexdump.h              # Hex/ASCII dump utility (debug)
├── Build / packaging
│   ├── Makefile               # Standard build + OpenWRT dual-mode (103 lines)
│   ├── Dockerfile             # Alpine-based Docker image
│   ├── debian/                # Debian package metadata (see Debian Packaging section)
│   │   ├── control            # Package metadata and dependencies
│   │   ├── changelog          # Release history (latest: 0.103-1)
│   │   ├── rules              # debhelper build rules
│   │   ├── install            # File installation list → /opt/hm/hmcfgusb/
│   │   ├── hmcfgusb.links     # Symlinks from /opt/hm/hmcfgusb/ to /usr/{s,}bin/
│   │   ├── hmland.init        # SysV init script for hmland daemon
│   │   ├── hmland.default     # /etc/default/hmland config (port, logging)
│   │   ├── hmland.logrotate   # Log rotation configuration
│   │   └── ...                # Example/template files (manpage, cron, etc.)
│   └── version.h              # Version string ("0.103-git")
├── Configuration
│   ├── hmcfgusb.rules         # udev rules for USB device access
│   └── .gitignore             # Ignores *.o, *.d, binaries, *.enc, *.eq3
├── Scripts
│   ├── enable-hmmodiprf.sh    # Load cp210x driver for HM-MOD-UART
│   ├── reset-hmmoduart.sh     # GPIO reset for HM-MOD-UART on Raspberry Pi
│   └── init.hmland.OpenWRT    # Procd init script for OpenWRT
├── CI/CD
│   └── .github/workflows/
│       ├── docker-image.yml   # Docker build + publish + signing
│       └── renovate.json5     # Automated dependency updates config
└── Documentation
    ├── README.md
    ├── CLAUDE.md              # This file
    └── LICENSE                # MIT
```

---

## Build System

### Standard Build (Linux/macOS)

```bash
# Install prerequisites (Debian/Ubuntu)
apt-get install libusb-1.0-0-dev build-essential clang

# Build all binaries
make

# Clean build artifacts
make clean
```

**Compiler:** Clang (default; override with `CC=gcc make`)
**Flags (x86_64):** `-MMD -O2 -march=x86-64-v3 -mtune=cannonlake -Wall -Wextra -g`
**Flags (other):** `-MMD -O2 -Wall -Wextra -g`
**Libraries:** `libusb-1.0`, `librt`
**Extra paths:** `-I/opt/local/include`, `-L/opt/local/lib`

**Note:** The Makefile detects the architecture via `uname -p` and applies `-march=x86-64-v3 -mtune=cannonlake` only on x86_64. ARM and other platforms (e.g. Raspberry Pi) get plain `-O2` automatically.

### Build Targets and Their Object Dependencies

| Binary | Object Files |
|--------|-------------|
| `hmland` | `hmcfgusb.o hmland.o util.o` |
| `hmsniff` | `hmcfgusb.o hmuartlgw.o hmsniff.o` |
| `flash-hmcfgusb` | `hmcfgusb.o firmware.o util.o flash-hmcfgusb.o` |
| `flash-hmmoduart` | `hmuartlgw.o firmware.o util.o flash-hmmoduart.o` |
| `flash-ota` | `hmcfgusb.o culfw.o hmuartlgw.o firmware.o util.o flash-ota.o hm.o aes.o` |

### OpenWRT / LEDE Build

The Makefile detects `OPENWRT_BUILD` and switches to OpenWRT's build system automatically. OpenWRT installs:
- `hmland` → `/usr/sbin/`
- `hmsniff`, `flash-hmcfgusb`, `flash-ota` → `/usr/bin/`
- Init script → `/etc/init.d/hmland`

### Docker Build

```bash
docker build -t hmcfgusb .
docker run -p 1234:1234 hmcfgusb   # runs hmland -v -p 1234 -I
```

The Dockerfile uses a **multi-stage build** on **Alpine Linux 3.23**.

- **Builder stage deps:** `build-base`, `clang`, `libusb-dev`
- **Final image deps:** `libusb`, `ca-certificates`
- **Binaries copied:** `hmland`, `hmsniff`, `flash-hmcfgusb`, `flash-hmmoduart`, `flash-ota` → `/app/hmcfgusb/`
- **Exposed port:** 1234
- **CMD:** `["/app/hmcfgusb/hmland", "-v", "-p", "1234", "-I"]`
- No source code or build tools present in the final image

### Debian Packaging

The `debian/` directory provides packaging for Debian-based systems.

- **Maintainer:** JSurf <jsurf@gmx.de>
- **Standards-Version:** 4.7.0; **debhelper-compat:** 13
- **Build-Depends:** `debhelper-compat (= 13)`, `libusb-1.0-0-dev`
- **Install path:** Binaries are installed to `/opt/hm/hmcfgusb/` with symlinks:
  - `hmland` → `/usr/sbin/hmland`
  - `hmsniff`, `flash-hmcfgusb`, `flash-hmmoduart`, `flash-ota` → `/usr/bin/`
- **Service management:** SysV init script (`hmland.init`) with defaults in `/etc/default/hmland`
- **Default config** (`hmland.default`): port 1000, logging disabled, `HMLAND_ENABLED="1"`
- **Log rotation:** Configured via `hmland.logrotate`
- **Latest changelog entry:** version 0.103-2 (modernised packaging)

To build a `.deb` package:
```bash
dpkg-buildpackage -us -uc
```

---

## Source Code Modules

### `hmcfgusb.c / .h` — USB Driver (632 lines)

Wraps libusb-1.0 for the HM-CFG-USB device.

- **USB IDs:** `0x1b1f:0xc00f` (normal mode), `0x1b1f:0xc010` (bootloader mode)
- **USB timeout:** `USB_TIMEOUT = 10000 ms`
- **Frame size:** 64 bytes async transfers at 32 ms intervals
- Key functions: `hmcfgusb_send()`, `hmcfgusb_poll()`, `hmcfgusb_enter_bootloader()`, `hmcfgusb_leave_bootloader()`, `hmcfgusb_enumerate()`

### `hmland.c` — LAN Emulation Daemon (920 lines)

TCP server that emulates the HomeMatic LAN adapter protocol.

- **Default port:** 1000 (configurable via `-p`)
- **CLI flags:**
  - `-D` — debug + verbose mode
  - `-d` — daemon mode (with PID file)
  - `-I` — impersonate HM-LAN-IF (for older Fhem / Homegear < 2015)
  - `-p <port>` — TCP listen port
  - `-l <logfile>` — log file path
  - `-v` — verbose
- Supports scheduled device reboots and logging to syslog/file with timestamps

### `hmsniff.c` — Packet Sniffer (404 lines)

Captures and displays raw BidCoS packets over the air. Works with HM-CFG-USB, HM-MOD-UART, and HM-LGW-O-TW-W-EU.

### `flash-ota.c` — Over-the-Air Flasher (1342 lines, most complex)

Firmware updater supporting multiple transport types.

- **Device types:** HM-CFG-USB, culfw-based (CUL/COC), TSCULFW, HM-MOD-UART
- **Payload sizes:** `NORMAL_MAX_PAYLOAD = 37`, `LOWER_MAX_PAYLOAD = 17`
- **Max retries:** `MAX_RETRIES = 5`
- **Features:** CRC16 validation, AES signing challenge-response, automatic bootloader entry via serial or network
- **CLI flags:**
  - `-f <file>` — firmware file (.eq3 format)
  - `-s <serial>` — device serial number
  - `-c <device>` — culfw serial device
  - `-U <device>` — HM-MOD-UART serial device
  - `-C <hmid>` — HMID of central (for automatic bootloader entry)
  - `-D <hmid>` — HMID of target device
  - `-K <idx:key>` — AES key for signed devices

### `hmuartlgw.c / .h` — UART Driver

Driver for HM-MOD-UART and HM-LGW-O-TW-W-EU serial devices.

- Frame escaping/unescaping for UART protocol
- Dual-mode: OS (bootloader) and App (normal operation)
- Commands: `UPDATE_FIRMWARE`, `CHANGE_APP`, `SEND`, `ADD_PEER`, `SET_HMID`, AES key management
- `struct hmuartlgw_dev` holds `struct termios oldtio`; `hmuartlgw_close()` restores terminal settings and frees the device struct
- `hmuartlgw_poll()` guards against receive buffer overflow on oversized frames

### `culfw.c / .h` — culfw Driver

Serial protocol driver for CUL-based devices. Supports baud rates: 9600, 19200, 38400, 57600, 115200 (default: 38400).

- `struct culfw_dev` holds `struct termios oldtio`; `culfw_close()` restores terminal settings and frees the device struct

### `hm.c / .h` — HomeMatic Protocol

Packet format macros and AES signing.

- **Packet macros:** `HM_LEN()`, `HM_MSGID()`, `HM_CTL()`, `HM_TYPE()`, `HM_PAYLOAD()`, `HM_SRC()`, `HM_DST()`
- **Device type enum:** `HMCFGUSB`, `CULFW`, `HMUARTLGW`
- **Functions:** `hm_sign()` — AES challenge-response; `hm_set_debug()` — enable debug hexdumps

### `aes.c / .h` — AES Cryptography (1096 lines)

Brad Conte's public domain AES implementation (AES-128, AES-192, AES-256, ECB/CBC modes). Used for device authentication.

### `firmware.c / .h` — Firmware Parser

Intel HEX and eq3 firmware file reader with CRC16 validation.

- Supported MCUs: ATmega328P (image size `0x7000`), ATmega644P (image size `0xF000`)
- Intel HEX: per-record checksum verified; `addr + len` bounds checked against image array
- File descriptor closed on all exit paths

### `util.c / .h` — Utilities

- `ascii_to_nibble()` / `nibble_to_ascii()` conversions (`nibble_to_ascii` masks input with `& 0xf`)
- `validate_nibble()` — returns 1 if byte is a valid hex ASCII character

---

## Development Conventions

### Code Style

This is a C codebase. Follow the existing style:
- **Indentation:** Tabs
- **Braces:** K&R style (opening brace on same line as control statement)
- **Naming:** `snake_case` for functions and variables, `UPPER_CASE` for macros and constants
- **Header guards:** `#ifndef FILENAME_H` / `#define FILENAME_H` / `#endif`
- Compiler warnings treated as errors conceptually (`-Wall -Wextra`) — keep builds warning-free

### Branching Strategy

- `main` — primary branch (stable, all merges go here)
- Feature/fix branches use the pattern `claude/<feature>-<id>` for AI-assisted work

### Commit Messages

Follow the conventional commits style used in this repo:
```
chore(deps): update alpine docker tag to v3.23 (#51)
chore(deps): update actions/checkout action ( v5.0.1 → v6.0.1 ) (#50)
```

For code changes, use descriptive imperative messages:
```
fix: correct AES channel-mask transmission for signed devices
feat: add support for HM-LGW-O-TW-W-EU network gateway
```

---

## CI/CD Pipeline

### Docker Image Workflow (`.github/workflows/docker-image.yml`)

**Triggers:**
- Push to any branch or tag
- Monthly scheduled build (14th of month, 00:00 UTC)

**Permissions:** `contents: read`, `packages: write`, `id-token: write` (for OIDC-based Cosign signing)

**Steps:**
1. Checkout repository
2. Login to ghcr.io and Docker Hub
3. Install Cosign for image signing
4. Setup QEMU and Docker Buildx
5. Generate image metadata and tags (targets both ghcr.io and DockerHub)
6. Build and push Docker image (`linux/amd64`, `linux/arm/v7`, `linux/arm64`) — push only on `main`, tags, and scheduled builds
7. Sign published images with Cosign using GitHub OIDC tokens

**Action versions (pinned by SHA, managed by Renovate):**
- `actions/checkout` v6.0.1
- `docker/login-action` v3.6.0
- `docker/metadata-action` v5.10.0
- `docker/setup-qemu-action` v3.7.0
- `docker/setup-buildx-action` v3.12.0
- `docker/build-push-action` v6.18.0
- `sigstore/cosign-installer` v4.0.0

**Image tags generated:**
- `edge` — latest dev build
- Branch name, PR number, commit SHA (long format)
- Semantic version tags on releases (major.minor)
- Date-based tags on scheduled builds (`YYYY-MM-DD`)

### Renovate (`.github/renovate.json5`)

Automated dependency updates extend from `local>olcond/renovate-config`.

---

## USB Device Access (udev Rules)

To allow non-root users to access the HM-CFG-USB device:

```bash
sudo cp hmcfgusb.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```

Rules grant access for:
- USB `0x1b1f:0xc00f` — HM-CFG-USB normal mode
- USB `0x1b1f:0xc010` — HM-CFG-USB bootloader mode

---

## Key Protocol Notes

### BidCoS Packet Format

Packets are accessed via macros in `hm.h`:
- Byte 0: `HM_LEN` — payload length
- Byte 1: `HM_MSGID` — message counter
- Byte 2: `HM_CTL` — control byte
- Byte 3: `HM_TYPE` — message type
- Bytes 4–6: `HM_SRC` — source device address
- Bytes 7–9: `HM_DST` — destination device address
- Bytes 10+: `HM_PAYLOAD` — message payload

### AES Signing

Devices with AES signing enabled (e.g. KeyMatic, door sensors) require challenge-response authentication. The `hm_sign()` function in `hm.c` handles this using `aes.c`. The `-K` flag in `flash-ota` takes `<index>:<128-bit-hex-key>`.

**Security note:** Versions before 0.101 had a bug where the AES channel-mask was not correctly transmitted to the HM-CFG-USB, potentially allowing unsigned messages. Version ≥ 0.101 is required for correct authenticated message handling.

### LAN Adapter Protocol Compatibility

- **Without `-I` flag:** hmland identifies as `HM-USB-IF` (correct for eQ-3 rfd, CCU, modern Fhem/Homegear)
- **With `-I` flag:** hmland impersonates `HM-LAN-IF` (required for Fhem before 2015-06-19 and Homegear before 2015-07-01)

---

## Typical Workflows

### Run hmland Server

```bash
# Debug mode (verbose, port 1234)
./hmland -p 1234 -D

# Daemon mode with log file
./hmland -d -p 1000 -l /var/log/hmland.log

# With LAN impersonation for older clients
./hmland -p 1000 -I
```

Then configure Fhem:
```
define hmusb HMLAN 127.0.0.1:1234
attr hmusb hmId <hmId>
```

### Flash HM-CFG-USB Firmware

```bash
# Download firmware from eQ-3, then:
./flash-hmcfgusb hmusbif.XXXX.enc
```

### Over-the-Air Device Firmware Update

```bash
# Via HM-CFG-USB (device serial KEQ0123456)
./flash-ota -f firmware.eq3 -s KEQ0123456

# Via CUL/COC serial device
./flash-ota -f firmware.eq3 -s KEQ0123456 -c /dev/ttyACM0

# Via HM-MOD-UART
./flash-ota -f firmware.eq3 -s KEQ0123456 -U /dev/ttyAMA0

# Automatic bootloader (with AES signing)
./flash-ota -f firmware.eq3 -C ABCDEF -D 012345 -K 01:00112233445566778899AABBCCDDEEFF
```

### Sniff HomeMatic Traffic

```bash
./hmsniff
```

### Reset HM-MOD-UART (Raspberry Pi)

```bash
./reset-hmmoduart.sh  # Uses GPIO18
```

---

## Dependencies

### Build Dependencies

| Dependency | Purpose |
|-----------|---------|
| `libusb-1.0-dev` | USB device communication |
| `clang` (or `gcc`) | C compiler |
| `make` | Build system |

### Runtime Dependencies

| Dependency | Purpose |
|-----------|---------|
| `libusb-1.0` | USB device access at runtime |
| `librt` | POSIX real-time extensions |

### Standard C Libraries Used

`stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `unistd.h`, `errno.h`, `fcntl.h`, `termios.h`, `sys/socket.h`, `netinet/in.h`, `arpa/inet.h`, `time.h`, `signal.h`, `math.h`

---

## File Modification Guidelines for AI Assistants

1. **Do not modify `aes.c`** — it is a third-party public domain implementation. Keep it as-is.
2. **`version.h`** contains a single `#define VERSION` string. Update it when bumping the version.
3. **`hmcfgusb.rules`** — udev rules; edit with care as these affect hardware access permissions.
4. **`Makefile`** — dual-mode (standard + OpenWRT). Changes to compilation flags must not break either mode.
5. **Header files (`.h`)** — define public APIs between modules. Changing function signatures requires updating all callers.
6. **`flash-ota.c`** is the most complex file (1342 lines). It handles multiple device types with separate code paths — trace carefully before modifying.
7. **Protocol correctness is critical** — incorrect packet construction or AES signing changes can silently break device communication or security guarantees.
8. **Maintain `-Wall -Wextra` cleanliness** — the build flags treat warnings seriously; introduce no new warnings.
9. **No test suite exists** — validate changes manually with actual hardware or by code review.
10. **`debian/` packaging** — the install paths and symlinks in `debian/install` and `debian/hmcfgusb.links` must be updated if new binaries are added.
11. **`.gitignore`** — already covers `*.o`, `*.d`, compiled binaries, and firmware files (`*.enc`, `*.eq3`). Add new build artifacts here when introducing new targets.

---

## Known Discrepancies and Notes

No outstanding discrepancies. All previously noted issues have been resolved:
- Dockerfile multi-stage build (PR #55), ARM platform support (PR #56/#57)
- Makefile arch flag conditional restored (PR #56)
- Debian packaging modernised to Standards-Version 4.7.0 / debhelper-compat 13 (PR #60)
