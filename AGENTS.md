# AGENTS.md

Guidance for AI coding agents (Claude, Codex, Copilot, Gemini, Cursor, Aider, …) working in this
repository. Applies to the whole repo; deeper-nested instructions, if any, take precedence.

## Project overview

Monitoring solution for a DAIKIN/ROTEX Solaris RPS3/RPS4 solar thermal controller:

- `esphome/components/daikin_rotex_solaris/` — the ESPHome **external component** (Python codegen +
  C++ runtime) that parses the controller's serial data stream on an ESP32 and exposes sensors.
- `esphome/solaris-template.yaml` — the reference ESPHome device configuration (committed).
- `ha-dashboard/` — a Home Assistant [`ha-floorplan`](https://github.com/ExperienceLovelace/ha-floorplan)
  dashboard (SVG + CSS + YAML rules) plus `convert-svg.py`.
- `drawio/` — editable sources (`solaris-dashboard.drawio` for the dashboard SVG,
  `wiring-schema.drawio`, `solaris.drawio`) for the diagrams in `img/`.
- `manuals/` — vendor PDFs (RPS3/RPS4 DE+EN, level shifter) — the protocol reference.
- `img/` — screenshots/diagrams referenced by the READMEs.

Hardware path: Solaris 3.5 mm stereo jack (Tip=Tx, Ring=Rx, Sleeve=GND) → 5 V↔3.3 V level shifter →
ESP32 UART RX (default `GPIO1`), 9600 baud, no parity. Controller must be set (technician code `0110`)
to `System → Data output`: Cycle 5 s, Record `AD-232`, Baudrate 9600, Address 255.

There is **no unit-test suite and no CI workflow**. Validation = ESPHome config validation + compile,
and for the dashboard, visual verification in Home Assistant. Licensed under GPL-3.0.

## Environment, build & validation

A local `venv` (Python 3.12, per `.python-version`; auto-activated by `.envrc` via direnv) ships the
ESPHome CLI.

```bash
source venv/bin/activate           # or: direnv allow  (.envrc does this)
cd esphome
esphome config  solaris.yaml       # fast schema validation (does NOT run to_code / codegen)
esphome compile solaris.yaml       # full build; runs to_code and regenerates solaris_error_codes.h
esphome run     solaris.yaml       # compile + OTA/serial upload
esphome logs    solaris.yaml       # remote logs
../cleanup.sh                      # purge .esphome + __pycache__ (resolves its own dir, runnable from anywhere)
```

Always run at least `esphome config` after touching component Python code or YAML, and
`esphome compile` after touching C++ or anything that affects codegen (only `compile` executes
`to_code`, which is where the error-codes header is produced).

`convert-svg.py` needs no dependencies beyond the standard library:

```bash
python ha-dashboard/convert-svg.py input.svg -o solaris-rps-dashboard.svg
```

### Local vs. committed files

`esphome/solaris.yaml` and `esphome/secrets.yaml` are **gitignored local working copies**; create them
from `solaris-template.yaml` and `secrets-template.yaml`. Functional changes belong in
`solaris-template.yaml` (the committed file) and should be mirrored into the local copy for testing.
The two files differ on purpose:

- template → `external_components: - source: github://milkotodorov/daikin-rotex-solaris-rps@main`
- local dev → `- source: {type: local, path: components}`

`esphome/components/daikin_rotex_solaris/solaris_error_codes.h` is also gitignored — it is generated.

Secrets are referenced only via `!secret`. Both `solaris-template.yaml` and `secrets-template.yaml`
use `wifi_ssid`, `wifi_password`, `solaris_ap_fallback_password`, `solaris_encryption_key`,
`solaris_web_server_username`, `solaris_web_server_password`. Never commit real values.

### Component YAML surface

```yaml
daikin_rotex_solaris:
  id: daikin_rotex_solaris_component
  uart_id: uart_bus        # from uart.UART_DEVICE_SCHEMA
  language: de             # optional, default "de"; de|en|fr|it|es
  solaris_tk:              # every Solaris sensor is optional — omit it and it is never instantiated
    id: solaris_tk
    filters:
      - throttle: 10s
    web_server:
      sorting_weight: 1
  git_hash:                # required; exposes the component's git commit hash as a diagnostic text sensor
    name: "Git Hash"
    web_server:
      sorting_weight: 71
```

Conventions in `solaris-template.yaml`: fast-changing values get `throttle: 10s`, slow ones
(`solaris_tr`, `solaris_ts`) `throttle: 30s`; binary and error sensors are unthrottled.
`web_server.sorting_weight` orders the WebUI — 1-11 for Solaris entities, 60+ for ESP diagnostics.
Entity names come from the component's translations, so do **not** set `name:` in YAML —
the one exception is `git_hash`, which has no translation and requires an explicit `name:`.
The device also exposes ESP health entities (internal temperature, uptime, heap, WiFi signal,
device info, restart button) and `web_server` v3 with `local: true` and basic auth.

### Testing without hardware

Uncomment the `DEBUG SIMULATION` block at the top of `DaikinRotexSolarisComponent::loop()`
(`daikin_rotex_solaris.cpp`). It feeds a synthetic line (`"0;1;75;0;84;58;61;63;3,2;;3500"`) into
`parse_line_()` every 10 s and returns early, bypassing UART.

## Firmware architecture

### Serial protocol

The controller emits one semicolon-delimited line per cycle:

```
Ha;BK;P1;P2;TK;TR;TS;TV;DF;Err;P      e.g.  0;1;75;0;84;58;61;63;3,2;;3500
```

Field order is fixed and encoded in the `SolarisFields` enum in `daikin_rotex_solaris.h`
(`TOTAL_FIELDS = 11`): manual operation, burner contact, circulation pump %, booster pump,
collector/return/storage/flow temperatures (°C), flow rate (l/min), error code, power (W).

Quirks handled in `parse_line_()` — preserve them:

- Flow rate uses a **comma** decimal separator; it is rewritten to `.` before `strtof`.
- `Err` is a single character (`''`, `K`, `R`, `S`, `V`, `D`, `G`, `F`, `W`); empty = no error.
- Power arrives in **Watts** and is published in **kW** (`publish_values_`: ÷1000, rounded to 2 dp).
- Boot/info banner lines (`SOLARIS`, `Zyklus`, `HA;BK;P1`) are detected and ignored.
- Lines are rejected unless length is within `MIN_LINE_LEN..MAX_LINE_LEN` **and** the semicolon token
  count is exactly `TOTAL_FIELDS`.
- `LINE_TIMEOUT_MS` (5 s) discards a stale partial buffer; after `OFFLINE_TIMEOUT_MS` (90 s) of
  silence `invalidate_all_sensors_()` publishes `NAN` / `invalidate_state()` so HA shows N/A rather
  than stale values.

### Data-driven sensor definitions

`sensors_config.py` holds the single source of truth **for the protocol sensors**: `SENSORS_CONFIG`,
a list of dicts with `type` (`numeric` | `binary` | `text`), `key`, `display_name` lambda, C++
`setter` name, and unit/icon/device_class/state_class/accuracy. (The `git_hash` diagnostic sensor is
the exception — it is not protocol-derived, so it is declared directly in `__init__.py`'s
`CONFIG_SCHEMA` and wired in `to_code()`, outside this list.)

`sensors.py` derives *both* the config schema (`SENSORS_SCHEMA`, consumed by `CONFIG_SCHEMA` in
`__init__.py`) and the runtime entity creation from that list, dispatching on `type` to
`sensor.new_sensor` / `binary_sensor.new_binary_sensor` / `text_sensor.new_text_sensor` and wiring the
entity to C++ via `getattr(parent, cfg['setter'])(sens)`. Never hand-write per-sensor schema code.

Two-phase naming: the schema is built at import time using `DEFAULT_LANGUAGE` names; `setup_sensors()`
later overwrites `CONF_NAME` with the language selected in YAML (falling back via
`translation_exists()`).

### Build-time code generation

`__init__.py::to_code()` writes `components/daikin_rotex_solaris/solaris_error_codes.h` on every
compile, containing only the selected language's `ERROR_CODES[]` table (with `_cpp_escape()` applied
to descriptions). That file is auto-generated and gitignored — **never edit it**; change
`translations/<lang>.py` instead. The `unknown` entry must remain last, since `get_error_text_()`
relies on `UNKNOWN_ERROR_INDEX == ERROR_CODES_COUNT - 1` as the fallback.

### C++ conventions

- No dynamic allocation in the hot path: reuse `conversion_buffer_` and `error_msg_buffer_`.
- Guard verbose logging with `#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_…`; use the file-level `TAG`.
- Protected members and internal methods carry a trailing underscore (`buffer_idx_`, `parse_line_()`).
- Sensor pointers are `{nullptr}`-initialised and every publish site is null-checked.
- New **protocol** sensors (driven by the UART stream) must be added to `dump_config()`,
  `publish_values_()` **and** `invalidate_all_sensors_()`. Static sensors that publish once (e.g.
  `git_hash`, set in `setup()`) belong in `dump_config()` only — they have no per-cycle publish and
  are not invalidated when the controller goes offline.

## Home Assistant dashboard (`ha-dashboard/`)

| File | Role |
| ---- | ---- |
| `solaris-rps-dashboard.svg` | Converted floorplan image (already ha-floorplan ready) |
| `solaris-rps-dashboard.css` | Colour/size/visibility/animation classes |
| `solaris-rps-dashboard.yaml`| `custom:floorplan-card` rules binding HA entities to SVG element ids |
| `convert-svg.py` | draw.io SVG → ha-floorplan SVG converter |

Deployment is manual: copy the `.svg` and `.css` into `/config/www/floorplans/solaris-rps/` (HACS +
`ha-floorplan` installed), then paste the YAML into a view or a manual card. The card's `image:` and
`stylesheet:` paths must match the chosen `/local/...` folder.

### SVG id conventions

draw.io cell ids become the hooks the YAML rules target:

- `<key>_txt` — static label (P1, P2, Tk, Tr, Ts, Tv, V, P, Bk, H); the `floorplan.text_set` value
  mirrors the text already drawn in the SVG
- `<key>_val` — value text, updated via `floorplan.text_set`
- `cell-<id>` — the draw.io group/shape wrapping an element, targeted to show/hide or style the
  surrounding box (e.g. `cell-bk_txt`, `cell-p2_led`, `cell-wasser_hoch`, `cell-wasser_runter`)

Every rule must target an id that actually exists in the SVG — a typo silently does nothing, with no
error in Home Assistant.

### Rule conventions

- Every value rule uses the same guard so offline data renders greyed-out `N/A`:
  `'${entity.state === "unknown" || entity.state === "unavailable" ? "N/A" : …}'`
- Formatting matches the firmware precision: `parseInt` + `" °C"`, `toFixed(1) + " l/min"`,
  `toFixed(2) + " kW"`, `parseInt + "%"`; binary sensors render localized `ein`/`aus`.
- Colour coding: temperatures/flow blue (`txt-blue`), collector temperature red (`txt-red`), pump
  state green (`txt-green`), power/manual-mode yellow (`txt-yellow`), unavailable grey (`txt-grey`).
- Static labels get `non-clickable`; conditional indicators toggle `visible` / `hidden`.
- Pipe flow is animated by applying `flow-animation visible` to `cell-wasser_hoch` /
  `cell-wasser_runter` while the circulation pump value is `> 0`.
- Entity ids follow the German ESPHome names of the default language, e.g.
  `sensor.esp_rotex_solaris_rps3_kollektortemperatur`,
  `binary_sensor.esp_rotex_solaris_rps3_brennerkontakt`. Changing the device name, the
  `friendly_name`, or the component `language:` changes these ids — the dashboard YAML must be
  updated in lockstep.

### Regenerating the SVG

Only needed after editing `drawio/solaris-dashboard.drawio`. Export with the draw.io `svgdata` plugin
enabled so `data-cell-id` attributes survive, then run `convert-svg.py`, which replaces draw.io
`<foreignObject>` nodes with native `<text>` elements (carrying the cell id, computed x/y, font and
fill taken from the inline CSS), drops empty containers, and emits `<tspan>`s for multi-line labels.

## Cross-cutting conventions

- **Adding or changing a protocol sensor touches five places**: `sensors_config.py`; all five
  `translations/*.py`; the enum, setter, and member in `daikin_rotex_solaris.h`; the
  `dump_config`/`publish_values_`/`invalidate_all_sensors_` blocks in `daikin_rotex_solaris.cpp`; and
  `solaris-template.yaml` (plus the dashboard YAML/SVG if it should be visualised). Non-protocol
  diagnostic sensors like `git_hash` bypass this — see *Data-driven sensor definitions* and *C++
  conventions*.
- **Translations are all-or-nothing**: `de`, `en`, `fr`, `it`, `es` each export `SENSOR_NAMES_<LANG>`
  and `ERROR_CODES_<LANG>` with identical key sets; new languages must be registered in both maps in
  `translations/translations.py`. `de` is `DEFAULT_LANGUAGE` and the fallback.
- **Docs are bilingual**: each `README.md` is German and its `README.en.md` sibling is the English
  mirror, both starting with the de/en badge header. Update both (repo root and `ha-dashboard/`).
- **RPS4 differences** (no booster pump): handled by commenting out the `solaris_p2` block in the
  ESPHome YAML and the `p2_*` rules in the dashboard YAML. Keep those documented comment blocks intact.
- Keep the existing heavy section-banner comment style (`# ===== SECTION =====`) in Python, C++ and
  YAML — it is used consistently across the codebase.
- Safety framing matters: the READMEs carry an explicit warranty/liability warning. Do not weaken it,
  and do not add anything that writes to the controller — this project is **read-only monitoring**
  (UART RX only; no TX pin is configured).
