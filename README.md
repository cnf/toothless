# Toothless

Reflow-oven / dryer controller firmware for ESP32, built on ESP-IDF (C++23).

## Features

- PID temperature control with autotune
- Reflow profile and drying-cycle modes, with safety interlocks (thermal
  runaway, stall detection, sensor-fault/timeout checks)
- Pluggable sensor/actuator peripherals, self-registered and auto-probed at boot
- Settings persisted to NVS via a central `ConfigManager`, with per-key
  validation (min/max/enum/length)
- WiFi + HTTP server for status, OTA firmware upload, log streaming, and file
  browsing
- Optional LVGL touchscreen UI, selectable per board via Kconfig

## Supported hardware

Board support is selected at build time via `menuconfig` /
`CONFIG_IMPL_*` (see `main/Kconfig.projbuild`):

Not all boards are equally well supported.

- Breakout board
- Lilygo T-Display S3 Long
- Lilygo T-HMI S3
- Waveshare Touch LCD 4
- Waveshare 3.5" RPi (G)
- Cheap Yellow Board 4.3"
- M5Stack Dial

## Prerequisites

- ESP-IDF >= 5.5.0
- [devenv](https://devenv.sh/) (recommended — pulls in ESP-IDF, `espflash`,
  and `esptool` via the Nix flake in `devenv.nix`/`devenv.yaml`), **or**
- the provided `.devcontainer` (VS Code Dev Containers, pinned to IDF
  `v5.5.1`)

## Building

```bash
# enter the dev environment (devenv) or open in the dev container
idf.py set-target esp32s3
idf.py menuconfig   # select board under "Toothless" > hardware implementation
idf.py build
idf.py -p <PORT> flash monitor
```

Board-specific `sdkconfig.*` fragments live under `config/` and partition
tables under `config/partitions*.csv`.

## Project layout

```
main/                 Application code (heater, peripherals, UI, helpers)
components/           In-tree ESP-IDF components (networking, config
                       manager, io_manager, pubsub_bridge, board impls)
managed_components/   Dependencies pulled by the IDF component manager
config/               sdkconfig fragments and partition tables
notes/                Design/architecture notes
```

## Status

Actively developed; expect rough edges. Known gaps are called out in the
architecture diagram (e.g. `pubsub_bridge` is built but not yet wired up for
multi-board UART splits).


See [`toothless-architecture.html`](./toothless-architecture.html) for a
diagram of how the pieces fit together.



