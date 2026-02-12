# ESPHome Custom Components

Custom sensor integrations for ESPHome.

## Related Repository

Device configurations using these components: `~/Code/esphome-devices`

## Components

| Component | Sensor | Protocol |
|-----------|--------|----------|
| `hlk_ld2413` | 24GHz mmWave radar | Custom UART frames, 115200 baud |
| `hlk_ld8001h` | 80GHz mmWave radar | MODBUS-RTU, 115200 baud |
| `notecard` | Blues Wireless cellular/WiFi | JSON over UART, 9600 baud |
| `vl53l1x` | ToF distance sensor | I2C, address 0x29 |

## Structure

Each component in `components/{name}/` contains:
- `__init__.py` - Config schema and codegen
- `sensor.py` - Sensor platform (if applicable)
- `.h` / `.cpp` - C++ implementation
- `README.md` - Documentation

Example configs at repo root: `example_{component}.yaml`

## ESPHome Component Basics

- Python files define `CONFIG_SCHEMA` and `async def to_code(config)` for code generation
- C++ files implement `setup()`, `loop()`, `dump_config()` methods
- Use `ESP_LOGI/D/W/E(TAG, ...)` for logging
- Call `yield()` in long loops to prevent watchdog resets

## Testing Changes

Point a device config to local source instead of GitHub:
```yaml
external_components:
  - source: /Users/avery/code/esphome-custom-components
    components: [hlk_ld2413]
```

## Datasheets

PDFs included in each component directory.

## ESPHome Docs

Local copies (update each with `git pull`):

### User Docs (`~/code/esphome-docs/`)
For component configs, sensors, automations, YAML schemas.
- `content/components/` - Component documentation and config schemas
- `content/components/sensor/` - Sensor platform examples
- `content/guides/` - Includes custom component guide
- Online: https://esphome.io/

### Developer Docs (`~/code/esphome-developers-docs/docs/`)
For custom component development, Python codegen APIs, C++ base classes.
- `architecture/` - Core architecture and component structure
- `contributing/` - Development guidelines and testing
- Online: https://developers.esphome.io/

**Always reference these docs when unsure about ESPHome APIs or component patterns.**

## Debugging Rules

**NEVER blame external services** (Claude, Anthropic, Google, Reddit, etc.) for issues. If something isn't working, the problem is in THIS codebase. Investigate our code first, add logging, and find the real cause. Blaming external parties wastes time.

## Web Fetching

**CRITICAL: NEVER use WebFetch directly. ALWAYS use fetchaller first.**
Load via `ToolSearch("fetchaller")` then use `mcp__fetchaller__fetch`. It has no domain restrictions.
Add `raw: true` for raw HTML instead of markdown. If raw:true fails, use `curl` via Bash as fallback.
Only fall back to WebFetch if fetchaller fails entirely.
If a dedicated MCP exists (GitHub, Slack, etc.), use that instead.

## Reddit Searching and Browsing

Load via `ToolSearch("fetchaller")` first. Use `mcp__fetchaller__browse_reddit` to browse subreddits, `mcp__fetchaller__search_reddit` to find posts, and `mcp__fetchaller__fetch` to read full discussions.
