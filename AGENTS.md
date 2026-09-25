# Liquid-handler project preferences

## Generated G-code

The user requests that all future generated G-code release all stepper motors
when each motion/operation sequence finishes. Append `M400` (wait for all
queued movement) followed by `M84` (disable all steppers). Apply this to the
end of generated jobs and intentional idle pauses / completed jog operations;
do not insert releases between segments of a continuous motion sequence.
Do not silently replace this with holding current or leave motors enabled.
If capturing a taught position, use `M400`, `M114`, then `M84`.

The syringe/plunger is axis A (Marlin's internal I axis), not E. On the
configured Creality V4.2.2 board its enable pin is shared with X/Y/Z.
Moving any axis therefore energizes all drivers; independent A-axis release
is not available through the existing shared enable wiring.

## Position teaching and communication

This build has no configured axis position encoders. Physically pushing a
released carriage/plunger does not update Marlin's coordinates. Teach using
motor-driven jogs from a reproducible homed reference. Released axes can
shift without detection; recapture/re-home as appropriate before trusting
coordinates. G92 assigns coordinates and does not measure physical motion.

The teaching controller is in m5stack-controller/. Its USB Host Shield 2.0
CH340 driver passed full firmware identification and ten consecutive position
queries with acknowledged motor release. USB reads must be capped to one
endpoint packet to avoid losing replies on a subsequent NAK. The older
m5stack-usb-probe TinyUSB implementation has truncated/stalled replies and
must not be used for motion control. See m5stack-controller/README.md.

Do not bind Core button-A to raw M400/M84 during an active job. That shortcut
caused false connection faults during homing and repeated out-of-band replies.
Front-panel buttons are currently unassigned; releases use the job queue.
