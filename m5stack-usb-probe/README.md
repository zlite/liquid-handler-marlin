# M5Stack Core Basic → Ender USB diagnostic

Hardware communication was demonstrated on 2026-09-24. The Core Basic with
USB Module V1.2 detects the Ender's CH340 (`1a86:7523`) at 250000 baud and
receives Marlin Liquid Handler R5 identification and position replies.
**Long replies currently lose bytes and later replies can stall with the pinned USB library. This is a
communication proof, not a reliable machine-control implementation.**

## Required V1.2 switches

Set these with power off; enable only one route in each switch bank.

| Bank | CH1 | CH2 | CH3 |
| --- | --- | --- | --- |
| SS | OFF (G13) | ON (G5) | OFF (G0) |
| INT | ON (G35) | OFF (G34) | — |

On the user's board/photo, ON is left. INT top G34 is OFF and bottom G35 ON.
SS top G0 is OFF, middle G5 ON, bottom G13 OFF.
The original wrong switch settings prevented USB-chip detection (`0xff`)
and made the screen dark, even with standalone display firmware. Correcting
the switches restored the screen and MAX3421E revision reads (`0x13`).
No EN pin modification is needed for this Basic + V1.2 setup.

## Connections and behavior

- Computer → Core USB-C: upload and diagnostic serial at 115200 baud.
- Module USB-A → Ender USB input; Ender powered normally.
- Module SPI: SCK 18, MOSI 23, MISO 19, CS 5, interrupt 35.
- Ender UART: 250000 baud, 8N1, matching `../marlin/Marlin/Configuration.h`.
- LCD: USB status, scrolling diagnostic text, uptime and received-byte count.
- Button A restarts the diagnostic.
- LCD and USB share one SPI object; SD is disabled.

Each USB attachment sends `M115` (identification), `M114` (position), and
`M119` (endstops), three seconds apart. No motion, homing, heating, or
persistent configuration commands are sent. Computer serial input is not
forwarded. The diagnostic does not implement acknowledgement/retry handling
and its green serial-connected status indicates USB serial mounting, not
validated complete Marlin replies.

## Build / upload

Run from `/home/chris/liquid-handler`:

```sh
pio run -d m5stack-usb-probe -e m5stack
pio run -d m5stack-usb-probe -e m5stack -t upload --upload-port /dev/ttyUSB0
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

The default `m5stack` diagnostic is currently installed. The USB library is
pinned to commit `29544f5c0e118c8bb5f895fccfa8ce9141e8ce1b`. The library is left unmodified; experimental buffering/service changes were
reverted after they failed to establish reliable complete replies.

## Verification and remaining limitation

Build and upload succeed on ESP32-D0WDQ6, 4 MiB flash, USB serial `01BBCF65`.
The host reads MAX3421E revision `0x13` at SPI speeds of 1, 4, 8, and 26 MHz,
then enumerates CH340 and mounts serial at 250000 baud.
The initial switch-corrected test received:

- `FIRMWARE_NAME:Marlin bugfix-2.1.x Liquid Handler R5 (Sep 23 2026 16:11:09)`
- `X:0.00 Y:0.00 Z:0.00 A:0.00 Count X:0 Y:0 Z:0 A:0`, followed by `ok`.
- Endstop response text, with truncation in the longer reply.

Evidence: `../artifacts/m5stack-usb-probe-switches-fixed-2026-09-24.log` and
`../artifacts/m5stack-ender-communication-2026-09-24.log` (final firmware).

Long reply bursts lose bytes; the final retest received a partial M115 reply
but no M114/M119 reply within the capture window. The earlier position/ok
response proves bidirectional communication but not reliable sustained use.
Larger buffers alone did not solve it. The
library's ESP32 interrupt task has a 5 ms delay; shortening/removing it and
trying polling/background service did not yield reliable complete replies.
Those experimental library patches were reverted. Before automated control,
resolve the USB receive issue, verify full replies including acknowledgements
across repeated commands/reconnects, and add an acknowledgement-driven queue.
Motion and dispensing were not tested.

## Standalone screen test

```sh
pio run -d m5stack-usb-probe -e screen-test -t upload --upload-port /dev/ttyUSB0
```

Cycles red/green/blue/white every three seconds, with GPIO32 backlight forced
HIGH, no USB host driver and no SD. Button presses log to serial. The user
confirmed colors with the module removed, and again with it attached after
correcting the switches.

## Original firmware backup

Before the first upload, the complete 4 MiB flash was saved privately to
`../artifacts/m5stack-original-2026-09-24.bin`. Keep it local because it may
contain saved settings/credentials. Restore from the project root with:

```sh
/home/chris/.platformio/penv/bin/esptool --port /dev/ttyUSB0 --baud 460800 write-flash 0 artifacts/m5stack-original-2026-09-24.bin
```

References:
- https://docs.m5stack.com/en/module/USB%20v1.2%20Module
- https://github.com/m5stack/M5-Max3421E-USBShield
