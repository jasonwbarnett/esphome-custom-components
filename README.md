# ESPHome Custom Components

This repository contains custom components for [ESPHome](https://esphome.io/), a system to control ESP32 devices with simple YAML configuration files.

## Available Components

-   [HLK-LD2413](#hlk-ld2413) - 24GHz mmWave radar liquid level sensor
-   [HLK-LD8001H](#hlk-ld8001h) - 80GHz mmWave radar liquid level sensor
-   [Notecard](#notecard) - Blues Wireless Notecard IoT modules
-   [VL53L1X](#tof400c-vl53l1x) - Time-of-Flight distance sensor (one-shot ranging)
-   [VL53L1X (Pololu port)](#tof400c-vl53l1x-pololu-port) - Time-of-Flight distance sensor (continuous ranging)

## HLK LD2413

A budget precision liquid level detection sensor using 24GHz millimeter wave radar technology with a detection range of 0.15m to 10.5m and accuracy of ±3mm under optimal conditions. The component enforces all datasheet constraints, including detection range (150mm-10500mm) and report cycle timing (50ms-1000ms).

The LD2413 is very picky about surroundings and reflections. Enclosures or electronics too close to it can cause it to not provide any readings (or provide wildy inaccurate ones). Always test it solo first to help narrow down any issues.

### Installation

```yaml
external_components:
    - source: github://jasonwbarnett/esphome-custom-components
      components: [hlk_ld2413]
```

**Resources:**

-   [Documentation](components/hlk_ld2413/README.md)
-   [Example Configuration](example_hlk_ld2413.yaml)
-   Purchase ($20CAD): [AliExpress](https://www.aliexpress.com/item/1005006766564668.html) / [AliExpress 2](https://www.aliexpress.com/item/1005008479449270.html)

## HLK-LD8001H

A high-end precision liquid level detection sensor using 80GHz millimeter wave radar technology with a detection range of 0.15m to 40m and accuracy of ±5mm under optimal conditions. This UART component supports both distance-only mode and water depth calculation when installation height is specified.

The 80GHz technology offers excellent detection capabilities even in challenging environments, making it ideal for water tanks, silos, and other liquid level monitoring applications. It is not picky about enclosures and works fine with a 3D printed case or electronics nearby.

There's an optional lens that narrows the beamwidth to ±3 degrees, whereas without the lens and just the bare sensor it is quite wide at ±25 degrees.

### Installation

```yaml
external_components:
    - source: github://jasonwbarnett/esphome-custom-components
      components: [hlk_ld8001h]
```

**Resources:**

-   [Documentation](components/hlk_ld8001h/README.md)
-   [Example Configuration](example_hlk_ld8001h.yaml)
-   Purchase ($50CAD): [AliExpress](https://www.aliexpress.com/item/1005006703020398.html) / [Alibaba](https://www.alibaba.com/product-detail/HLK-LD8001H-80G-liquid-level-detection_1601053500911.html)

## Blues Notecard

Integration with Blues Wireless Notecard (both cellular and WiFi variants) for IoT connectivity. Features include configurable data collection and sync intervals, automatic data batching, and access to Notecard temperature and battery voltage.

The implementation focuses specifically on using the Notecard for data transmission to Notehub, while the ESP32 handles sleep cycles and data filtering for optimal power efficiency.

### Installation

```yaml
external_components:
    - source: github://jasonwbarnett/esphome-custom-components
      components: [notecard]
```

**Resources:**

-   [Documentation](components/notecard/README.md)
-   [Example Configuration](example_notecard.yaml)
-   Purchase ($70CAD): [Blues Shop](https://shop.blues.com/collections/notecard)

## TOF400C VL53L1X

Integration with the VL53L1X/VL53L4CD Time-of-Flight (ToF) distance sensor. Supports both VL53L1X and VL53L4CD with configurable distance modes, a selectable **ranging mode** (one-shot or continuous), and range status reporting.

This I2C component adds **in-place self-healing**: it detects a frozen or I2C-locked sensor and re-initialises it without rebooting the device, exposing an optional recovery counter for diagnostics.

> **Want continuous ranging? Use [VL53L1X (Pololu port)](#tof400c-vl53l1x-pololu-port) instead.** This driver's `continuous` mode one-frame-halts on at least one tested TOF400C — traced to this driver's init sequence, not the hardware. Use this component for **one-shot** ranging (the default).

### Installation

```yaml
external_components:
    - source: github://jasonwbarnett/esphome-custom-components
      components: [vl53l1x]
```

**Resources:**

-   [Documentation](components/vl53l1x/README.md)
-   [Example Configuration](example_vl53l1x.yaml)

## TOF400C VL53L1X (Pololu port)

A second VL53L1X integration that vendors the well-tested [Pololu `vl53l1x-arduino`](https://github.com/pololu/vl53l1x-arduino) driver (transport swapped to ESPHome I2C), driving the sensor in **continuous (autonomous) ranging** at up to ~50 Hz.

Use this when you want continuous ranging. It sustains continuous mode (~10 Hz, verified) on the same TOF400C where the `vl53l1x` driver one-frame-halts — the chip is genuine ST, the difference is the driver. Configurable distance mode / timing budget / inter-measurement period, in-place freeze recovery with a recovery counter, and an optional `frames` proof-of-life sensor.

### Installation

```yaml
external_components:
    - source: github://jasonwbarnett/esphome-custom-components
      components: [vl53l1x_pololu]
```

**Resources:**

-   [Documentation](components/vl53l1x_pololu/README.md)

## General Information

### Features

-   **Easy Integration**: Simple YAML configuration for complex sensors
-   **Fully Documented**: Comprehensive documentation for each component
-   **Example Configurations**: Ready-to-use example files for quick setup

### Note on Installation

Using the GitHub repository version means you'll automatically get updates, but it could change at any moment. If stability is critical for your project, consider using the local installation method described in each component's documentation.

## License

This project is licensed under the MIT License with Attribution - see the [LICENSE](LICENSE) file for details.
