# VL53L1X Component for ESPHome

This component provides integration with the VL53L1X/VL53L4CD Time-of-Flight (ToF) distance sensor for ESPHome using I2C. This implementation is an adaptation of existing open-source code that has been modified to enhance functionality and compatibility with ESPHome.

## Features

-   Supports both VL53L1X and VL53L4CD sensors
-   I2C communication
-   Configurable distance mode (short/long)
-   Configurable ranging mode (one-shot/continuous)
-   Configurable update interval
-   Range status reporting
-   Automatic sensor initialization and configuration
-   Self-healing: detects a frozen or I2C-locked sensor and re-initialises it **in place** (no device reboot), with an optional recovery counter for diagnostics

## Hardware Setup

Connect your VL53L1X/VL53L4CD sensor to your ESP32/ESP8266 using the following pins:

-   VL53L1X VCC → ESP32 3.3V
-   VL53L1X GND → ESP32 GND
-   VL53L1X SDA → ESP32 SDA (GPIO21 in example)
-   VL53L1X SCL → ESP32 SCL (GPIO22 in example)
-   VL53L1X XSHUT → ESP32 GPIO (GPIO23 in example, optional for power control)

## Configuration Variables

-   **distance_mode** (_Optional_, string, default: "long"): The distance mode of the sensor. Options are "short" or "long". Note that VL53L4CD sensors only support "short" mode and will be automatically configured as such.
-   **ranging_mode** (_Optional_, string, default: "one_shot"): How measurements are driven — see [Ranging modes](#ranging-modes). `one_shot` (recommended) has the host trigger and re-arm every measurement; `continuous` lets the sensor range autonomously.
-   **update_interval** (_Optional_, time, default: 0.5s): How often the measured values are published.
-   **i2c_id** (_Optional_, ID): The ID of the I2C bus if you have multiple I2C buses.

## Sensor Outputs

-   **distance**: Distance measurement in millimeters
-   **range_status**: Status of the range measurement (0 = valid, 1-4 = various error states)
-   **recovery_count** (_Optional_): Monotonic count of in-place self-heal recoveries since boot. A flat value means the sensor is healthy; a rising value means it keeps hanging but is auto-recovering. Exposed as a diagnostic sensor.

## Basic Configuration

```yaml
i2c:
    sda: GPIO21
    scl: GPIO22
    scan: true

# Optional XSHUT pin control
output:
    - platform: gpio
      pin: GPIO23
      id: xshut_pin

# Power on sequence
esphome:
    on_boot:
        then:
            - output.turn_on: xshut_pin
            - delay: 1s # Allow VL53L1X to boot (min 1.2ms required)
    on_shutdown:
        then:
            - output.turn_off: xshut_pin

vl53l1x:
    distance_mode: long
    ranging_mode: one_shot
    update_interval: 0.5s

sensor:
    - platform: vl53l1x
      distance:
          name: "VL53L1X Distance"
          id: tof_distance
      range_status:
          name: "VL53L1X Range Status"
      recovery_count:
          name: "VL53L1X Recoveries"
```

## Ranging modes

The sensor can be driven two ways via `ranging_mode`:

| Mode | How it works | Use when |
| --- | --- | --- |
| `one_shot` (default) | The component triggers a single measurement, reads it, then explicitly re-arms the next. | Always safe. Immune to the inter-measurement scheduler, and the only mode that works on some quirky modules. |
| `continuous` | The sensor ranges autonomously on its inter-measurement timer; the component just clears the interrupt after each frame. | Slightly less I2C traffic, on modules confirmed to sustain it. |

Per ST application note **UM2356 §2.2**, the inter-measurement period must be **greater than the timing budget + 4 ms** (the sensor's own API rejects anything else with `INVALID_PARAMS`). This component sets a valid period automatically.

> **Caveat:** even with a valid period, some VL53L1X clones — notably certain **TOF400C** boards — emit only a single frame in `continuous` mode and then halt, regardless of configuration. Watch `recovery_count`: if it climbs steadily in `continuous` mode, switch to `one_shot`. This is why `one_shot` is the default.

## Self-healing

The component watches for two failure modes and re-initialises the sensor **in place** (no device reboot, ~250 ms reconfigure) when it detects them:

-   **Frozen ranging** — no fresh measurement frame for ~5 s (the documented stuck/lock-up behaviour where the sensor keeps reporting the same value forever).
-   **I2C faults** — a streak of consecutive failed transactions (bus lock-up).

Each recovery increments the optional `recovery_count` sensor, so you can distinguish a healthy sensor (flat count) from one that keeps hanging (rising count) at a glance.

## Binary Sensors

In addition to the `sensor` platform, a `binary_sensor` platform exposes
threshold and validity sensors derived from the measured distance. The
`binary_sensor` platform requires the `sensor` platform to also be configured on
the same device (the I2C device is registered by the `sensor` platform).

-   **range_valid**: `on` when the latest measurement has a valid range status.
-   **above_threshold**: `on` when the measured distance is greater than
    `above_distance` (mm). Forced `off` while the range status is not valid.
-   **below_threshold**: `on` when the measured distance is less than
    `below_distance` (mm). Forced `off` while the range status is not valid.

```yaml
sensor:
    - platform: vl53l1x
      distance:
          name: "VL53L1X Distance"
          id: tof_distance

binary_sensor:
    - platform: vl53l1x
      range_valid:
          name: "VL53L1X Range Valid"
      above_threshold:
          name: "VL53L1X Above 1000mm"
          above_distance: 1000 # mm
      below_threshold:
          name: "VL53L1X Below 500mm"
          below_distance: 500 # mm
```

## Filtering Example

This example shows how to filter out invalid readings and apply a median filter to stabilize measurements:

```yaml
sensor:
    - platform: vl53l1x
      distance:
          name: "VL53L1X Distance"
          filters:
              - lambda: |-
                    if (x < 45 || x > 4200 || x == 0.0 || std::isnan(x)) {
                      return 0;
                    }
                    return x;
              - median:
                    window_size: 5
                    send_every: 5
                    send_first_at: 1
```

## Installation

There are two ways to install this component:

### Option 1: Direct from GitHub (Recommended)

Add the following to your ESPHome configuration:

```yaml
external_components:
    - source: github://jasonwbarnett/esphome-custom-components
      components: [vl53l1x]
```

For reproducible builds, pin to a specific commit or tag:

```yaml
external_components:
    - source:
          type: git
          url: https://github.com/jasonwbarnett/esphome-custom-components
          ref: main # or a commit SHA / tag
      components: [vl53l1x]
```

**Note:** Tracking a branch means you'll automatically get updates, but it could change at any moment. If stability is critical, pin to a commit/tag or use the local installation method.

### Option 2: Local Installation

If you prefer a more stable setup or need to modify the component:

1. Create a `components` directory in your ESPHome configuration directory
2. Copy the `vl53l1x` directory into the `components` directory
3. Add the `external_components` section to your YAML configuration:

```yaml
external_components:
    - source: components
```

## Notes

-   The VL53L1X component automatically detects and configures the sensor on startup
-   The VL53L4CD sensor is automatically detected and forced to use "short" distance mode
-   The timing budget is fixed at 500 ms (favouring repeatability), so the sensor produces roughly **two readings per second**
-   Minimum ranging distance is ~40 mm (per the datasheet): closer targets are still detected, but the reported distance is not accurate
-   In **long** distance mode the maximum range collapses under strong ambient light (≈360 cm in the dark vs ≈73 cm in bright sunlight) — in bright conditions a far target can read as "no target"/invalid rather than as a large distance. Consider this when interpreting an absent/empty reading outdoors or near an open door.
-   Measurement readiness is polled over I2C, so the XSHUT and interrupt pins are optional/unused
-   Readings with an invalid range status, or transient out-of-band values, should be filtered out (see the filtering example above)
