# SIM7080G ESPHome Component

Custom ESPHome component for the SIM7080G Cat-M/NB-IoT modem. Uses the modem's built-in HTTP(S) client via AT commands over UART — no PPP stack, no TinyGSM, no general-purpose cellular abstraction.

## Why This Component

- **Not a general modem component** — purpose-built for SIM7080G's AT command HTTP client
- **Same pattern as existing Notecard component** — UART in, UART out, command/response
- **Keeps everything in ESPHome** — reuse LD8001H radar component as-is, YAML config, familiar toolchain
- **Avoids oarcher's unmerged PPP modem PR** ([#6721](https://github.com/esphome/esphome/pull/6721)) — unstable, heavyweight, different approach entirely

## Target Hardware

### Development Board: SIM7080G Breakout (ANDPROG)

Primary dev board for building and testing the component in isolation.

| Spec | Value |
|------|-------|
| Board | [SIM7080G Breakout (AliExpress)](https://www.aliexpress.com) — ANDPROG, ~$29 CAD |
| Interface | TTL/UART |
| Power in | 5V-10V (5V recommended, from ESP32 dev board 5V pin or USB) |
| PWRKEY | GPIO controlled |
| GNSS | Supported (PCB GPS antenna option) |
| PMU | None — modem power always on when board is powered |

**Dev wiring (SIM7080G breakout + any ESP32 dev board):**

```
ESP32 dev board          SIM7080G breakout
─────────────────        ─────────────────
5V  ──────────────────→  PWRIN (5V)
GND ──────────────────→  GND
GPIO TX ──────────────→  RXD
GPIO RX ←──────────────  TXD
GPIO (any) ───────────→  PWRKEY
```

**Dev plan:** Build and test Phase 1-2 (AT command engine, HTTP POST) on this board. No PMU, no level conversion concerns — just UART + PWRKEY. Once the component works, move to LilyGO for deep sleep and battery integration.

**Note:** Listing says "Updated Firmware in random" — check `AT+CGMR` on first boot to confirm firmware version.

### Production Board: LilyGO T-SIM7080G-S3

All-in-one board for production deployments and deep sleep / battery testing.

| Component | Part | Notes |
|-----------|------|-------|
| Board | LilyGO T-SIM7080G-S3 | ESP32-S3 + SIM7080G + AXP2101 PMU |
| Modem | SIM7080G | Cat-M + NB-IoT, built-in HTTP(S) client |
| SIM | 1NCE (10yr/500MB) | APN: `iot.1nce.net` |
| PMU | AXP2101 | I2C power rail control |

### LilyGO Pin Definitions

```
BOARD_MODEM_PWR_PIN     GPIO41    PWRKEY (pulse to toggle power)
BOARD_MODEM_DTR_PIN     GPIO42    Sleep/wake control
BOARD_MODEM_RI_PIN      GPIO3     Ring indicator (input)
BOARD_MODEM_RXD_PIN     GPIO4     UART RX (ESP receives from modem)
BOARD_MODEM_TXD_PIN     GPIO5     UART TX (ESP sends to modem)
I2C_SDA                 GPIO15    PMU I2C data
I2C_SCL                 GPIO7     PMU I2C clock
PMU_INPUT_PIN           GPIO6     PMU interrupt
```

### LilyGO requires PMU init before modem comms

Unlike the dev board, the LilyGO needs AXP2101 I2C setup to power the modem:

```cpp
PMU.setDC3Voltage(3000);   // Modem power rail
PMU.enableDC3();
PMU.setBLDO1Voltage(3300);  // UART level conversion (critical!)
PMU.enableBLDO1();
```

Without BLDO1 enabled, UART to the modem won't work on LilyGO.

## Component Architecture

### File Structure

Follow ESPHome hub component pattern (like sim800l, modbus_controller):

```
components/sim7080g/
├── __init__.py              # Hub component: config schema + codegen
├── sim7080g.h               # Hub C++ header
├── sim7080g.cpp             # Hub C++ implementation (AT commands, power control)
├── sensor/
│   └── __init__.py          # Sensor platform (RSSI, SINR, etc.)
├── binary_sensor/
│   └── __init__.py          # Binary sensor platform (registered, connected)
└── text_sensor/
    └── __init__.py          # Text sensor platform (operator, network type, IP)
```

### Class Hierarchy

```
Component + uart::UARTDevice
    └── SIM7080G (hub)
            ├── SIM7080GSensor (sensor platform)
            ├── SIM7080GBinarySensor (binary_sensor platform)
            └── SIM7080GTextSensor (text_sensor platform)
```

### Existing Patterns to Follow

**sim800l** (closest match — GSM modem hub with sub-platforms):
- Hub component manages UART communication and state
- `sensor: platform: sim800l` for RSSI
- `binary_sensor: platform: sim800l` for registration status
- Actions: `sim800l.send_sms`, `sim800l.dial`
- Triggers: `on_sms_received`, `on_incoming_call`
- Docs: `~/code/esphome-docs/content/components/sim800l.md`

**Notecard** (our own — same UART command/response pattern):
- `send_command_and_get_response_()` with retries and timeouts
- `flush_rx_()` to clear UART buffer
- Blocking sequential init in `setup()`
- Empty `loop()` — command-based, not polling
- Source: `components/notecard/notecard.cpp`

**http_request** (action pattern for HTTP POST):
- Actions take URL, headers, body (templatable)
- `on_response` trigger with status_code access
- `on_error` trigger for network failures
- Docs: `~/code/esphome-docs/content/components/http_request.md`

## YAML Config Design

### Hub Component

```yaml
sim7080g:
  id: modem
  uart_id: modem_uart

  # Network
  apn: "iot.1nce.net"
  network_mode: catm           # catm (default) | nbiot | auto

  # Power control
  power_pin: GPIO41            # PWRKEY pin (pulse to toggle)
  dtr_pin: GPIO42              # DTR for sleep/wake
  status_pin: GPIO3            # RI pin (optional, input)

  # Timing
  registration_timeout: 60s    # Max wait for network registration
  command_timeout: 1s          # Default AT command timeout

  # Triggers
  on_connect:                  # Network registered + PDP active
    - logger.log: "Modem connected"
  on_disconnect:
    - logger.log: "Modem disconnected"
```

### Sensor Platform

```yaml
sensor:
  - platform: sim7080g
    sim7080g_id: modem
    rsrp:
      name: "Signal Power"     # dBm, from AT+CPSI?
    rsrq:
      name: "Signal Quality"   # dB, from AT+CPSI?
    sinr:
      name: "Signal to Noise"  # dB, from AT+CPSI?
```

### Binary Sensor Platform

```yaml
binary_sensor:
  - platform: sim7080g
    sim7080g_id: modem
    registered:
      name: "Network Registered"
```

### Text Sensor Platform

```yaml
text_sensor:
  - platform: sim7080g
    sim7080g_id: modem
    operator:
      name: "Operator"
    network_type:
      name: "Network Type"    # "LTE CAT-M1", "NB-IOT", etc.
    local_ip:
      name: "Local IP"
```

### Actions

```yaml
# Power management
- sim7080g.power_on:
    id: modem
- sim7080g.power_off:
    id: modem

# HTTP POST (main use case)
- sim7080g.http_post:
    id: modem
    url: "https://awire.ca/api/readings"
    headers:
      Content-Type: "application/json"
      X-Fleet-Key: "fk_..."
    body: !lambda |-
      return build_batch_json();
    on_response:
      - lambda: |-
          ESP_LOGI("modem", "HTTP %d", status_code);
    on_error:
      - logger.log: "HTTP request failed"

# Raw AT command (escape hatch)
- sim7080g.send_at:
    id: modem
    command: "AT+CSQ"
```

### Complete Example (cistern monitor)

```yaml
uart:
  - id: modem_uart
    tx_pin: GPIO5
    rx_pin: GPIO4
    baud_rate: 115200
    rx_buffer_size: 1024   # default 256 is too small for HTTP responses

  - id: radar_uart
    tx_pin: GPIO17
    rx_pin: GPIO16
    baud_rate: 115200

sim7080g:
  id: modem
  uart_id: modem_uart
  apn: "iot.1nce.net"
  power_pin: GPIO41
  dtr_pin: GPIO42
  network_mode: catm

sensor:
  - platform: hlk_ld8001h
    uart_id: radar_uart
    id: radar_distance
    distance:
      name: "Distance to Water"

  - platform: sim7080g
    rsrp:
      name: "Signal Power"

globals:
  - id: boot_count
    type: int
    restore_value: yes
    initial_value: '0'

deep_sleep:
  id: deep_sleep_1

esphome:
  on_boot:
    priority: -100
    then:
      - lambda: 'id(boot_count) += 1;'
      - if:
          condition:
            lambda: 'return id(boot_count) % 12 == 0;'
          then:
            - sim7080g.power_on:
                id: modem
            - sim7080g.http_post:
                id: modem
                url: "https://awire.ca/api/readings"
                headers:
                  Content-Type: "application/json"
                  X-Fleet-Key: "fk_..."
                body: !lambda 'return build_batch_payload();'
                on_response:
                  - logger.log: "Batch posted"
            - sim7080g.power_off:
                id: modem
      - deep_sleep.enter:
          id: deep_sleep_1
          sleep_duration: 2h
```

## C++ Implementation

### Hub Class (`sim7080g.h`)

```cpp
class SIM7080G : public Component, public uart::UARTDevice {
public:
    void setup() override;
    void loop() override;      // Empty — command-based like Notecard
    void dump_config() override;
    float get_setup_priority() const override { return setup_priority::DATA; }

    // Configuration setters (called by Python codegen)
    void set_apn(const std::string &apn);
    void set_network_mode(uint8_t mode);  // 1=Cat-M, 2=NB-IoT, 3=both
    void set_power_pin(GPIOPin *pin);
    void set_dtr_pin(GPIOPin *pin);
    void set_status_pin(GPIOPin *pin);
    void set_registration_timeout(uint32_t ms);

    // Actions (called by automations)
    bool power_on();
    bool power_off();
    bool http_post(const std::string &url,
                   const std::map<std::string, std::string> &headers,
                   const std::string &body,
                   int &status_code_out,
                   std::string &response_out);

    // State queries (for sensors/binary_sensors)
    bool is_registered();
    int get_rsrp();
    int get_rsrq();
    int get_sinr();
    std::string get_operator();
    std::string get_network_type();
    std::string get_local_ip();

protected:
    // AT command helpers (same pattern as Notecard)
    bool send_at_(const std::string &cmd);
    bool send_at_(const std::string &cmd, std::string &response,
                  uint32_t timeout = 1000);
    bool wait_for_ok_(uint32_t timeout = 1000);
    bool wait_for_response_(const std::string &prefix,
                            std::string &response,
                            uint32_t timeout = 1000);
    void flush_rx_();

    // Initialization sequence
    bool init_modem_();
    bool configure_network_();
    bool wait_for_registration_();
    bool activate_pdp_();

    // State
    std::string apn_;
    uint8_t network_mode_{1};  // Cat-M default
    GPIOPin *power_pin_{nullptr};
    GPIOPin *dtr_pin_{nullptr};
    GPIOPin *status_pin_{nullptr};
    uint32_t registration_timeout_{60000};
    bool registered_{false};
    bool pdp_active_{false};
};
```

### AT Command Flow

#### Power On Sequence

```
1.  Assert PWRKEY pin (LOW 100ms → HIGH 1000ms → LOW)
2.  Wait for "RDY" or poll AT until OK (up to 10s)
3.  AT+CLTS=1               → Enable network time sync (first boot only, saved with AT&W)
4.  AT+CNMP=38              → LTE only mode
5.  AT+CMNB=1 or 2          → Cat-M only / NB-IoT only
6.  AT+CGDCONT=1,"IP","apn" → Set APN
7.  Poll AT+CGREG? until registered (0,1 or 0,5)
8.  AT+CNACT=0,1            → Activate PDP context
9.  Wait for +APP PDP: 0,ACTIVE
10. AT+CCLK?                → Read network time, call settimeofday() on ESP32
```

#### HTTPS POST Sequence

Using HTTPS — the modem handles TLS internally. Awire's Caddy reverse proxy redirects HTTP→HTTPS, so HTTPS is required. Only 2 extra AT commands vs plain HTTP.

```
1.  AT+CSSLCFG="sslversion",1,3     → TLS 1.2
2.  AT+SHSSL=1,""                    → SSL config, no CA cert verification
3.  AT+SHCONF="URL","https://..."
4.  AT+SHCONF="BODYLEN",1024         → Set max body length (see size estimate below)
5.  AT+SHCONF="HEADERLEN",350        → Set max header length
6.  AT+SHCONN                        → Connect to server (TLS handshake here)
7.  AT+SHCHEAD                       → Clear any previous headers
8.  AT+SHAHEAD="Content-Type","application/json"
9.  AT+SHAHEAD="X-Fleet-Key","fk_..."
10. AT+SHBOD=<body>,<timeout_ms>     → Set request body (body string, timeout in ms)
11. AT+SHREQ="https://...",3         → Execute POST (3=POST)
12. Wait for +SHREQ: "POST",<status>,<response_length>
13. AT+SHREAD=0,<response_length>    → Read response body
14. AT+SHDISC                        → Disconnect
```

#### Payload Size Estimate

Cellular and location are top-level (sent once), not per-reading. Per-reading data is just the measurements:

```json
{"distance": 1523.5, "batteryVoltage": 3.72, "timestamp": 1736899200}
```

~70 chars per reading.

```
12 readings × 70 chars  = ~840 bytes
JSON wrapper + cellular + location (once) = ~300 bytes
Total ≈ ~1,140 bytes
```

Comfortably under the default `BODYLEN` of 1024 if using Unix timestamps instead of ISO strings, or set to 2048 for margin. Well within modem capabilities.

#### Power Off Sequence

```
1. AT+SHDISC         → Disconnect HTTP (if connected)
2. AT+CNACT=0,0      → Deactivate PDP context
3. AT+CPOWD=1        → Graceful power down
4. Wait for "POWER DOWN" response
```

#### Network Info (AT+CPSI?)

```
+CPSI: LTE CAT-M1,Online,302-720,0x1234,67890,450,EUTRAN-BAND4,2300,5,5,-85,-670,-440,15
       ↑rat        ↑mcc ↑mnc ↑tac   ↑cid        ↑band         ↑earfcn  ↑rsrp ↑rsrq  ↑sinr

Parse indices:
  [0]  System mode: "LTE CAT-M1" or "LTE NB-IOT"
  [1]  Operation mode: "Online"
  [2]  MCC-MNC: "302-720" (split on dash)
  [3]  TAC: hex → decimal
  [4]  Cell ID: decimal
  [6]  Band: "EUTRAN-BAND4"
  [7]  EARFCN: decimal
  [10] RSRP: dBm (direct)
  [11] RSRQ: dB (value ÷ 10)
  [13] SINR: dB (direct)
```

## Investigated: Timestamps

**Approach: Network time sync + ESP32 RTC persistence across deep sleep.**

### How It Works

1. **`AT+CLTS=1` + `AT&W`** — Enable automatic network time sync on the SIM7080G, saved to modem flash. After registration, the modem's internal clock syncs from the cellular network (NITZ). This only needs to be set once.

2. **`AT+CCLK?`** — Read the modem's clock after registration. Returns: `+CCLK: "YY/MM/DD,HH:MM:SS±ZZ"`. Parse this into a Unix timestamp.

3. **`settimeofday()`** — Set the ESP32-S3 system time from the modem's clock on transmit wakes. The ESP32's RTC timer **persists across deep sleep** (only lost on power-on reset). After setting time once, `gettimeofday()` returns correct time on all subsequent wakes.

4. **RTC drift** — The internal 150kHz RC oscillator drifts with temperature, but over 2-hour deep sleep intervals the error is negligible (seconds at most). An external 32kHz crystal improves accuracy at +1µA deep sleep cost, but is unnecessary for this use case.

### Boot Sequence for Timestamps

```
First boot (power-on):
  → No valid time yet
  → Force a transmit wake to sync time from network
  → AT+CCLK? → parse → settimeofday()
  → Now ESP32 RTC has correct time

Measure-only wakes (11 of 12):
  → gettimeofday() returns correct time (RTC persisted)
  → Store reading with timestamp

Transmit wakes (1 of 12):
  → Re-sync from AT+CCLK? to correct any drift
  → POST batch with per-reading timestamps
```

### References
- [ESP-IDF System Time (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/system_time.html) — confirms RTC persists across deep sleep
- [AT+CLTS network time sync](https://www.raviyp.com/gsm-network-time-synchronization-for-rtc-using-at-clts-command/) — AT+CLTS=1, AT&W to persist

## Investigated: Batch Storage

**Approach: `RTC_DATA_ATTR` struct array in the SIM7080G component.**

### Why RTC Memory (not globals)

| Option | Survives deep sleep | Flash wear | ESPHome integration |
|--------|-------------------|------------|-------------------|
| `globals` + `restore_value: yes` | Yes (NVS flash) | 12 writes/day = fine | Native YAML |
| `RTC_DATA_ATTR` | Yes (RTC slow memory) | Zero (SRAM) | Lambda only |
| NVS API direct | Yes (flash) | Same as globals | Lambda only |

Both globals and RTC memory work at 12 writes/day. **Use `RTC_DATA_ATTR`** inside the component's C++ code — it's the cleanest since the component owns the storage lifecycle and the data is small.

### Storage Structure

```cpp
struct StoredReading {
    float distance;        // or angle for propane
    float battery_voltage;
    float temperature;     // optional, for propane
    uint32_t timestamp;    // Unix epoch from gettimeofday()
    bool valid;            // slot has data
};

RTC_DATA_ATTR StoredReading stored_readings[48];  // Max batch size per awire API
RTC_DATA_ATTR int reading_count = 0;
RTC_DATA_ATTR bool time_synced = false;
```

Total: 48 × ~20 bytes = ~960 bytes. ESP32-S3 has 8KB RTC slow memory — fits easily.

The component exposes methods to store and retrieve readings, and builds the JSON batch payload internally. The user's YAML just calls `sim7080g.store_reading` on measure wakes and `sim7080g.http_post` on transmit wakes.

### Boot Counter

Use an ESPHome `globals` with `restore_value: yes` for the boot counter (single int, negligible flash wear). The counter drives the measure-vs-transmit decision in `on_boot`.

## Investigated: Payload Field Availability

**All fields in both water and propane payloads are available.**

### Water Payload

| Field | Source | How | Status |
|-------|--------|-----|--------|
| `deviceId` | ESP32-S3 MAC | `get_mac_address()` — last 6 hex chars | Available |
| `type` | Static | YAML config string | Available |
| `readings[].distance` | HLK-LD8001H radar | Existing ESPHome component, MODBUS register 0x0001 | Available |
| `readings[].batteryVoltage` | AXP2101 PMU (LilyGO) or ADC (dev board) | Separate sensor, passed via lambda | Available |
| `readings[].timestamp` | Network time + ESP32 RTC | AT+CCLK? → settimeofday() → gettimeofday() | Available (see above) |
| `cellular.rat` | AT+CPSI? field[0] | Parse "LTE CAT-M1" → "catm" | Available |
| `cellular.band` | AT+CPSI? field[6] | Parse "EUTRAN-BAND4" → "LTE BAND 4" | Available |
| `cellular.mcc` | AT+CPSI? field[2] | Parse "302-720" → 302 | Available |
| `cellular.mnc` | AT+CPSI? field[2] | Parse "302-720" → 720 | Available |
| `cellular.tac` | AT+CPSI? field[3] | Hex string → decimal | Available |
| `cellular.cid` | AT+CPSI? field[4] | Decimal | Available |
| `cellular.earfcn` | AT+CPSI? field[7] | Decimal | Available |
| `cellular.rsrp` | AT+CPSI? field[10] | dBm direct | Available |
| `cellular.rsrq` | AT+CPSI? field[11] | Value ÷ 10 for dB | Available |
| `cellular.sinr` | AT+CPSI? field[13] | dB direct | Available |
| `location.latitude` | Static config (fixed install) | YAML config | Available |
| `location.longitude` | Static config (fixed install) | YAML config | Available |

### Propane Payload (additions)

| Field | Source | How | Status |
|-------|--------|-----|--------|
| `readings[].angle` | Tilt/angle sensor | Separate sensor platform, passed via lambda | Available |
| `readings[].temperature` | Temp sensor | Separate sensor platform, passed via lambda | Available |

All other fields (deviceId, type, cellular, location) are identical to water.

### Notes on Specific Fields

**`batteryVoltage`** — Not part of the SIM7080G component. On LilyGO, the AXP2101 PMU reads battery voltage via I2C (`PMU.getBattVoltage()`). On dev boards without PMU, use an ADC pin with voltage divider. Either way, it's a separate sensor passed into the POST body lambda.

**`location`** — For fixed installations (cistern, propane tank), hardcode in YAML config. The SIM7080G does have built-in GNSS (`AT+CGNSINF`), but GPS fix takes 30-60s cold start and wastes power for stationary sensors. GNSS support can be added later for mobile use cases.

**`deviceId`** — Derived from ESP32 MAC address, not the SIM or modem. Available via ESPHome's `get_mac_address()` helper or `ESP.getEfuseMac()`.

## Deep Sleep + Batching Strategy

### Boot Counter Pattern

```
globals:
  - id: boot_count (int, restore_value: yes)

On every boot:
  1. boot_count += 1
  2. Power radar → read distance → sim7080g.store_reading(distance, voltage, timestamp)
  3. If boot_count % 12 == 0 (or first boot for time sync):
       power_on modem → sync time → POST batch → clear stored readings → power_off modem
  4. Enter deep sleep for 2 hours
```

### Wakeup Cause Detection

```cpp
int cause = esp_sleep_get_wakeup_cause();
// 0 = ESP_SLEEP_WAKEUP_UNDEFINED (first boot / power cycle → force transmit for time sync)
// 4 = ESP_SLEEP_WAKEUP_TIMER (normal deep sleep wake)
```

Use in `on_boot` to detect first boot vs timer wake. First boot forces a transmit cycle to sync time from the network before storing any readings.

## Power Management

### AXP2101 PMU Rails (LilyGO T-SIM7080G-S3)

| Rail | Voltage | Purpose | Component Controls |
|------|---------|---------|-------------------|
| DC3 | 3.0V | SIM7080G modem | Disable in sleep |
| BLDO1 | 3.3V | UART level conversion | Disable with modem |
| BLDO2 | 3.3V | GPS antenna | Disable (not used) |
| VSYS | 3.5-4.2V | Direct battery | Always available |

**BLDO1 is critical** — must be enabled for UART communication with modem. The LilyGO sleep example shows this: disable BLDO1 = no modem comms.

### PMU Control Options

**Option A: XPowersLib (Arduino library, I2C)**
```cpp
PMU.setDC3Voltage(3000);
PMU.enableDC3();   // Modem on
PMU.enableBLDO1(); // Level conversion on
// ... use modem ...
PMU.disableDC3();  // Modem off
PMU.disableBLDO1();
```

**Option B: GPIO PWRKEY only (simpler, no I2C dependency)**
```cpp
// Toggle power via PWRKEY pulse
digitalWrite(POWER_PIN, LOW);
delay(100);
digitalWrite(POWER_PIN, HIGH);
delay(1000);
digitalWrite(POWER_PIN, LOW);
```

For the ESPHome component, start with **Option B** (GPIO only). PMU control can be added later as optional I2C configuration. This keeps the component hardware-agnostic — works on LilyGO, custom PCBs, and Waveshare boards.

### Radar Power Control (LilyGO)

Separate from modem — uses GPIO + MOSFET on VSYS rail:
```
VSYS (3.5-4.2V) → MOSFET → HLK-LD8001H
                     ↑
                  GPIO control
```

This is handled outside the SIM7080G component, via a standard ESPHome `switch` + `output` on the MOSFET GPIO.

## Implementation Phases

### Phase 1: Core Hub + AT Commands
- [ ] Scaffold component file structure (`__init__.py`, `.h`, `.cpp`)
- [ ] Implement UART AT command send/receive (from Notecard pattern)
- [ ] Implement power on sequence (PWRKEY GPIO)
- [ ] Implement network registration (AT+CNMP, AT+CMNB, AT+CGREG polling)
- [ ] Implement PDP activation (AT+CNACT)
- [ ] Implement power off sequence (AT+CPOWD)
- [ ] Test on LilyGO board with 1NCE SIM

### Phase 2: HTTP POST Action
- [ ] Implement AT+SH* HTTP client sequence
- [ ] Expose `sim7080g.http_post` action with URL, headers, body
- [ ] Implement `on_response` trigger with status_code
- [ ] Implement `on_error` trigger
- [ ] Test POST to awire API

### Phase 3: Sensor Platforms
- [ ] Implement AT+CPSI? parsing for network info
- [ ] Add `sensor` platform: RSRP, RSRQ, SINR
- [ ] Add `binary_sensor` platform: registered
- [ ] Add `text_sensor` platform: operator, network_type, local_ip

### Phase 4: Deep Sleep Integration
- [ ] Test with ESPHome deep_sleep component
- [ ] Implement boot counter pattern with globals
- [ ] Implement RTC memory batch storage
- [ ] Test full cycle: boot → measure → conditional POST → sleep
- [ ] Validate power consumption in sleep

### Phase 5: Polish
- [ ] Add `sim7080g.send_at` escape hatch action
- [ ] Add optional PMU I2C control (AXP2101)
- [ ] Add configurable registration timeout
- [ ] Write README.md with wiring diagrams
- [ ] Add example YAML config

## Reference Links

### SIM7080G Documentation
- [AT Command Manual](https://www.waveshare.com/w/upload/3/39/SIM7080_Series_AT_Command_Manual_V1.02.pdf)
- [HTTP(S) Application Note](https://www.waveshare.com/w/upload/9/97/SIM7080_Series_HTTP%28S%29_Application_Note_V1.01.pdf)
- [Low Power Mode Application Note](https://www.waveshare.com/w/upload/a/aa/SIM7080_Series_Low_Power_Mode_Application_Note_V1.01.pdf)
- [Hardware Design Guide](https://curtocircuito.com.br/datasheet/modulo/SIM7080G_Hardware_Design.pdf)
- [Waveshare Wiki (AT examples)](https://www.waveshare.com/wiki/SIM7080G_Cat-M/NB-IoT_HAT)

### LilyGO T-SIM7080G-S3
- [GitHub repo (examples + schematics)](https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7080G)
- [HTTP/HTTPS example](https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7080G/tree/master/examples/SIM7080G-ATT-NB-IOT-HTTP-HTTPS) — confirms UART at 115200, Serial1 on GPIO4/5
- [Sleep example](https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7080G/tree/master/examples/MinimalModemAndEspSleep) — PMU rail management, PWRKEY sequence, DTR sleep control
- [XPowersLib (AXP2101)](https://github.com/lewisxhe/XPowersLib) — PMU library used by examples

### ESPHome Reference (local docs)
- **sim800l docs**: `~/code/esphome-docs/content/components/sim800l.md` — hub + sub-platform pattern to follow
- **deep_sleep docs**: `~/code/esphome-docs/content/components/deep_sleep.md` — actions, wakeup causes
- **globals docs**: `~/code/esphome-docs/content/components/globals.md` — restore_value persistence
- **http_request docs**: `~/code/esphome-docs/content/components/http_request.md` — action pattern for HTTP
- **UART dev docs**: `~/code/esphome-developers-docs/docs/architecture/components/uart.md` — UARTDevice pattern
- **Component architecture**: `~/code/esphome-developers-docs/docs/architecture/components/index.md` — lifecycle, priorities
- **Automations dev docs**: `~/code/esphome-developers-docs/docs/architecture/components/automations.md` — triggers, actions, conditions
- **Starter component (automations example)**: [esphome/starter-components/empty_automation](https://github.com/esphome/starter-components/tree/main/components/empty_automation) — reference implementation for `@automation.register_action`, `register_condition`, triggers with callbacks

### ESPHome Modem PR (reference only, different approach)
- [PR #6721 — PPP modem component](https://github.com/esphome/esphome/pull/6721) — 239 comments, unmerged, unstable as of 2026.1
- [PR #9801 — network plumbing (merged)](https://github.com/esphome/esphome/pull/9801) — foundation work, merged July 2025
- [Feature request: SIM7080G](https://github.com/esphome/feature-requests/issues/1379) — open since 2021

### 1NCE Connectivity
- [1NCE Dev Hub API](https://help.1nce.com/dev-hub/reference/api-welcome)
- APN: `iot.1nce.net`

### Related Projects in This Repo
- `components/notecard/` — same UART command/response pattern, use as code template
- `components/hlk_ld8001h/` — radar sensor, will be used alongside this component
- Design doc: `~/Code/temp/esp32-modem.md` — full hardware/power/API specs

## Key Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| PPP vs AT HTTP client | AT HTTP client (`AT+SH*`) | Simpler, lower power, no TCP/IP stack overhead |
| TinyGSM vs raw AT | Raw AT commands | Fewer dependencies, full control, HTTP is ~15 AT commands |
| General modem vs SIM7080G-specific | SIM7080G-specific | Avoid abstraction layers, match Notecard pattern |
| Power control default | GPIO PWRKEY | Hardware-agnostic; PMU I2C optional later |
| Framework | ESPHome (not Arduino/PlatformIO) | Reuse LD8001H component, YAML config, same ecosystem |
| HTTP vs HTTPS | HTTPS (no CA verify) | Caddy redirects HTTP→HTTPS so HTTPS required. Modem handles TLS internally, only 2 extra AT commands |
| Batch storage | `RTC_DATA_ATTR` struct array | Survives deep sleep, zero flash wear, component owns lifecycle |
| Timestamps | AT+CCLK? → settimeofday() | Network time on transmit wake, ESP32 RTC persists across deep sleep |
| Timestamp format | Unix epoch (uint32_t) | Smaller payload than ISO strings, server can convert |
| Location | Static YAML config | Fixed installations; GNSS support deferred for mobile use cases |
| Battery voltage | Separate sensor (not in SIM7080G component) | AXP2101 on LilyGO, ADC on dev boards — different per hardware |
