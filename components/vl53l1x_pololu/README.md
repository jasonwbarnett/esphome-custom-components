# VL53L1X (Pololu port) for ESPHome

An ESPHome component that vendors the well-tested [Pololu `vl53l1x-arduino`](https://github.com/pololu/vl53l1x-arduino)
driver, with its transport layer swapped from Arduino `Wire` to ESPHome's
`i2c::I2CDevice`. The register/ranging logic is byte-for-byte ST/Pololu; only the
6 I2C primitives and the result block-read were changed.

## Why this exists

The companion [`vl53l1x`](../vl53l1x) component drives the sensor fine in
**one-shot** mode but its **continuous** (autonomous) mode stalls after a single
frame on at least one module (a TOF400C). To find out whether that was the chip
or the driver, this component ports a known-good reference implementation.

**Result:** on the same TOF400C board, the Pololu sequence runs **continuous
ranging without issue (~10 Hz, live jitter, valid status)** — proving the chip is
fine and the stall is specific to the other driver's continuous path, not the
hardware. The chip also fingerprints as genuine ST (`model 0xEA`, `type 0xCC`,
`rev 0x10`).

So this component is useful if you want **reliable continuous ranging** on a
module where the minimal-ULD driver won't sustain it.

## Configuration

```yaml
external_components:
  - source: github://jasonwbarnett/esphome-custom-components
    components: [vl53l1x_pololu]

i2c:
  sda: GPIO21
  scl: GPIO22

vl53l1x_pololu:
  update_interval: 1s

sensor:
  - platform: vl53l1x_pololu
    distance:
      name: "ToF Distance"
    frames:                 # optional diagnostic: total ranging frames since boot
      name: "ToF Frames"
```

The component currently configures Long distance mode, a 50 ms timing budget, and
starts continuous ranging at a 100 ms inter-measurement period (these are sensible
defaults for the A/B test; they can be promoted to YAML options if needed).

## Notes / TODO

- Distance mode, timing budget, and inter-measurement period are hardcoded in
  `setup()` for now; exposing them as config keys is a natural next step.
- `Wire`/Arduino calls (`millis`/`delay`/`delayMicroseconds`) resolve to the
  `esphome::` equivalents via the enclosing namespace, so it builds on both the
  arduino and esp-idf frameworks.

## License

`vl53l1x_core.{h,cpp}` are derived from Pololu's library and ST's VL53L1X API,
licensed **BSD-3-Clause** — see [LICENSE.txt](LICENSE.txt). The ESPHome wrapper
and codegen are under the repository's license.
