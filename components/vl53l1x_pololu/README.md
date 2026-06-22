# VL53L1X (Pololu port) for ESPHome

An ESPHome component that vendors the well-tested [Pololu `vl53l1x-arduino`](https://github.com/pololu/vl53l1x-arduino)
driver, with its transport layer swapped from Arduino `Wire` to ESPHome's
`i2c::I2CDevice`. The register/ranging logic is byte-for-byte ST/Pololu; only the
6 I2C primitives and the 17-byte result block-read were adapted.

It drives the sensor in **continuous (autonomous) ranging** and adds in-place
**freeze recovery** so an unattended sensor self-heals without rebooting.

## When to use this vs. [`vl53l1x`](../vl53l1x)

| | [`vl53l1x`](../vl53l1x) | **`vl53l1x_pololu`** (this) |
|---|---|---|
| Ranging | one-shot (host triggers each frame) | **continuous (~up to 50 Hz)** |
| Basis | minimal ST ULD subset | full Pololu / ST low-power-auto driver |
| Continuous on a TOF400C | one-frame-halts ❌ | **sustains ~10 Hz ✅** |
| Extras | self-heal, `recovery_count`, binary-sensor thresholds | self-heal, `recovery_count`, `frames` |

**Use this component if you want continuous ranging.** The minimal `vl53l1x`
driver's continuous mode halts after one frame on at least one tested module — see
[Why this exists](#why-this-exists).

## Features

- Continuous (autonomous) ranging using Pololu's low-power-auto sequence
- Configurable **distance mode**, **timing budget**, and **inter-measurement period**
- In-place **freeze recovery**: re-initialises the sensor if frames stop arriving
  (no device reboot), counted via an optional `recovery_count` sensor
- Optional `frames` diagnostic — a proof-of-life counter that climbs while ranging
- Builds on both the **arduino** and **esp-idf** frameworks

## Wiring

| VL53L1X | ESP32 |
|---|---|
| VIN | 3.3V |
| GND | GND |
| SDA | GPIO21 (example) |
| SCL | GPIO22 (example) |

XSHUT and the interrupt (GPIO1) pins are not required — data readiness is polled
over I2C.

## Configuration

```yaml
external_components:
  - source: github://jasonwbarnett/esphome-custom-components
    components: [vl53l1x_pololu]

i2c:
  sda: GPIO21
  scl: GPIO22

vl53l1x_pololu:
  distance_mode: long       # short | medium | long   (default: long)
  timing_budget: 50ms       # 20ms–1000ms             (default: 50ms)
  inter_measurement: 100ms  # must be > timing_budget  (default: 100ms)
  update_interval: 1s       # how often values are published

sensor:
  - platform: vl53l1x_pololu
    distance:
      name: "ToF Distance"
      id: tof_distance
    frames:                 # optional: total ranging frames since boot (proof-of-life)
      name: "ToF Frames"
    recovery_count:         # optional: in-place freeze recoveries since boot
      name: "ToF Recoveries"
```

### Hub options (`vl53l1x_pololu:`)

- **distance_mode** (_Optional_, default `long`): `short` (≈1.3 m, most ambient-light
  immune), `medium`, or `long` (≈4 m in the dark; range drops sharply in bright
  ambient light).
- **timing_budget** (_Optional_, time, default `50ms`): integration time per
  measurement, `20ms`–`1000ms`. Higher = longer range and better repeatability, at
  lower frame rate and higher power.
- **inter_measurement** (_Optional_, time, default `100ms`): the continuous-ranging
  cadence. Per ST UM2356 it **must be greater than `timing_budget`** (validated at
  compile time); a value just above the budget gives the fastest rate.
- **update_interval** (_Optional_, default `60s`): how often the latest values are
  published to the front end. (Ranging itself runs continuously regardless.)
- standard `i2c` options (`address`, `i2c_id`).

### Sensor outputs (`sensor:` platform)

- **distance** (_Required_): distance in millimetres.
- **frames** (_Optional_, diagnostic): total ranging frames consumed since boot.
  Climbs steadily (~10/s at the defaults) while ranging is healthy; a flat line
  means the sensor stopped producing frames.
- **recovery_count** (_Optional_, diagnostic): number of in-place freeze recoveries
  since boot. Flat = healthy.

## Freeze recovery

If no fresh ranging frame arrives for ~5 s, the component performs an in-place
re-initialisation (`stopContinuous` → `init` → restart) — no device reboot, and the
recovery is counted on `recovery_count`. On a healthy sensor this never fires; it's
a safety net for the field.

## Why this exists

The companion [`vl53l1x`](../vl53l1x) component drives the sensor fine in
**one-shot** mode, but its **continuous** mode stalls after a single frame on at
least one module (a TOF400C). To find out whether that was the chip or the driver,
this component ports a known-good reference implementation.

**Result:** on the same TOF400C board, the Pololu sequence runs continuous ranging
without issue (**~10 Hz, live jitter, valid status**), proving the chip is fine and
the stall is specific to the other driver's continuous path — not the hardware. The
chip also fingerprints as genuine ST (`model 0xEA`, `type 0xCC`, `rev 0x10`). The
likely cause is the minimal-ULD init vs. Pololu's fuller DataInit/StaticInit + DSS
(low-power-auto) setup.

## Implementation notes

- Only the transport layer differs from upstream Pololu: the 6 register read/write
  primitives and the 17-byte result block-read call `i2c::I2CDevice` instead of
  Arduino `Wire`. Everything else is byte-for-byte ST/Pololu.
- `millis()`/`delay()`/`delayMicroseconds()` resolve to the `esphome::` equivalents
  via the enclosing namespace, so it works under both frameworks.

## License

`vl53l1x_core.{h,cpp}` are derived from Pololu's library and ST's VL53L1X API,
licensed **BSD-3-Clause** — see [LICENSE.txt](LICENSE.txt). The ESPHome wrapper and
codegen are under the repository's license.
