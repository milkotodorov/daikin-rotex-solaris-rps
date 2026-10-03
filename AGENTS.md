# AGENTS.md

Repo-wide; nested instructions take precedence.

**Hard constraints (don't break these):**
- **Read-only monitoring** — never add anything that writes to the controller (UART RX only, no TX pin).
- Don't weaken the warranty/liability warnings in the READMEs. GPL-3.0.
- `solaris-template.yaml` is committed; `solaris.yaml`/`secrets.yaml`/`solaris_error_codes.h` are not —
  never commit real secrets or edit the generated header.

## What this is

Monitoring for a DAIKIN/ROTEX Solaris RPS3/RPS4 solar thermal controller.

| Path | Role |
| ---- | ---- |
| `esphome/components/daikin_rotex_solaris/` | ESPHome **external component** (Python codegen + C++ runtime); parses the controller's serial stream on an ESP32 |
| `esphome/solaris-template.yaml` | Reference device config (**committed**) |
| `ha-dashboard/` | HA [`ha-floorplan`](https://github.com/ExperienceLovelace/ha-floorplan) dashboard (SVG + CSS + YAML) + `convert-svg.py` |
| `drawio/` | Editable sources for the diagrams in `img/` |
| `manuals/` | Vendor PDFs — the protocol reference |

Hardware: Solaris 3.5 mm jack (Tip=Tx, Ring=Rx, Sleeve=GND) → 5 V↔3.3 V level shifter → ESP32 UART RX
(`GPIO1`), 9600 baud, no parity. Controller set (tech code `0110`) to `System → Data output`: Cycle 5 s,
Record `AD-232`, Baud 9600, Address 255.

No unit tests, no CI. Validation = ESPHome config/compile; dashboard = visual check in HA.

## Build & validate

`venv` (Python 3.12, auto-activated by direnv via `.envrc`) ships the ESPHome CLI.

```bash
source venv/bin/activate            # or: direnv allow
cd esphome
esphome config  solaris.yaml        # fast schema check — does NOT run to_code/codegen
esphome compile solaris.yaml        # full build; runs to_code → regenerates solaris_error_codes.h
esphome run     solaris.yaml        # compile + OTA/serial upload
esphome logs    solaris.yaml
../cleanup.sh                       # purge .esphome + __pycache__ (runnable from anywhere)
```

- After touching component **Python/YAML**: run `esphome config`.
- After touching **C++ or anything affecting codegen**: run `esphome compile` (only `compile` runs
  `to_code`, which generates the error-codes header).
- `convert-svg.py` uses only the stdlib: `python ha-dashboard/convert-svg.py in.svg -o out.svg`.

**Test without hardware:** uncomment the `DEBUG SIMULATION` block at the top of `loop()` in
`daikin_rotex_solaris.cpp` — it feeds a synthetic line into `parse_line_()` every 10 s, bypassing UART.

## Local vs. committed files (gitignored working copies)

- `esphome/solaris.yaml`, `esphome/secrets.yaml`, and `.../solaris_error_codes.h` are **gitignored**.
  Create the first two from `solaris-template.yaml` / `secrets-template.yaml`; the header is generated.
- Functional changes go in **`solaris-template.yaml`** (committed), mirrored into local `solaris.yaml`
  for testing. The only intended diff: template uses `external_components: github://...@main`; local
  uses `source: {type: local, path: components}`.
- Secrets only via `!secret` (`wifi_ssid`, `wifi_password`, `solaris_ap_fallback_password`,
  `solaris_encryption_key`, `solaris_web_server_username`, `solaris_web_server_password`). Never commit
  real values.

## Component YAML surface

```yaml
daikin_rotex_solaris:
  uart_id: uart_bus
  language: de             # optional, default "de"; de|en|fr|it|es
  solaris_tk:              # every Solaris sensor is optional — omit it, it's never instantiated
    filters: [{ throttle: 10s }]
    web_server: { sorting_weight: 1 }
  git_hash:                # required diagnostic text sensor (git commit hash)
    name: "Git Hash"       # git_hash is the ONLY sensor that needs name: (it has no translation)
```

- **Do not set `name:`** on Solaris sensors — names come from translations. Exception: `git_hash`.
- `throttle`: fast values `10s`, slow (`solaris_tr`, `solaris_ts`) `30s`; binary/error sensors
  unthrottled.
- `sorting_weight`: 1–11 for Solaris entities, 60+ for ESP diagnostics.
- Device also exposes ESP health entities + `web_server` v3 (`local: true`, basic auth).

## Serial protocol (parsed in `parse_line_()`)

```
RPS3 (11 fields): Ha;BK;P1;P2;TK;TR;TS;TV;DF;Err;P              e.g. 0;1;75;0;84;58;61;63;3,2;;3500
RPS4 (13 fields): ...;P;DeltaT;Zust                             e.g. 0;0;0;0;51;45;49;43;0,0;;0;20;21
```

Field order is fixed in the `SolarisFields` enum (`daikin_rotex_solaris.h`): manual, burner contact,
circ pump %, booster pump, collector/return/storage/flow temps (°C), flow (l/min), error, power (W).
RPS4 adds `DeltaT` (field 12 — published as `solaris_deltat`) and `Zust`
(Betriebszustand: 23=active, 21=standby — logged only, not a sensor).
`solaris_deltat` is named neutrally "Spreizung" (spread): on RPS4 it is the controller's
**Sollspreizung** (calculated target DT from the T_K curve, per manual Illustration 5-2); on RPS3
the target is not transmitted so the component derives the **Istspreizung** (measured TV−TR).

**Quirks to preserve:**

- Flow uses a **comma** decimal separator → rewritten to `.` before `strtof`.
- `Err` is one char (`K R S V D G F W`, or empty = no error). `X` is a synthetic offline sentinel set
  by `invalidate_all_sensors_()`, **not** a controller code.
- Power arrives in **W**, published in **kW** (÷1000, 2 dp).
- Boot/banner lines (`SOLARIS`, `Zyklus`, `HA;BK;P1`) are detected and ignored.
- Lines rejected unless both length (`MIN/MAX_LINE_LEN`) and token count (11–13) are in range.
- Stale partial buffer cleared after `LINE_TIMEOUT_MS` (5 s); after `OFFLINE_TIMEOUT_MS` (90 s) of
  silence `invalidate_all_sensors_()` publishes `NAN`/`invalidate_state()` so HA shows N/A, not stale.

## Component internals

**Sensors are data-driven — never hand-write per-sensor schema code.** `sensors_config.py`'s
`SENSORS_CONFIG` (dicts: `type` `numeric|binary|text`, `key`, `display_name` lambda, `setter`, +
unit/icon/device_class/state_class/accuracy) is the single source of truth. `sensors.py` derives both
`SENSORS_SCHEMA` and runtime entity creation from it, dispatching on `type` and wiring to C++ via
`getattr(parent, cfg['setter'])(sens)`. Two-phase naming: schema built with `DEFAULT_LANGUAGE`;
`setup_sensors()` later overwrites `CONF_NAME` with the YAML-selected language (falling back via
`translation_exists()`).

**Error-codes header is generated.** `__init__.py::to_code()` writes `solaris_error_codes.h` each
compile (only the selected language's `ERROR_CODES[]`, via `_cpp_escape()`). Never edit it — change
`translations/<lang>.py`. The `unknown` entry **must stay last** (`get_error_text_()` uses
`UNKNOWN_ERROR_INDEX == COUNT - 1` as fallback).

**`solaris_tz`** (daily storage-temp gain, computed) accumulator lives in RAM — reboot resets the day
to 0 (intentional: no NVS, avoids flash wear; HA keeps history via `total_increasing`). HA action
`set_tz` → C++ `set_tz_acc()` restores it after a reboot.

**C++ conventions:**
- No heap in the hot path — reuse `conversion_buffer_` / `error_msg_buffer_`.
- Guard verbose logs with `#if ESPHOME_LOG_LEVEL >= …`; use file-level `TAG`.
- Protected members / internal methods carry a trailing underscore.
- Sensor pointers `{nullptr}`-init; null-check every publish site.

## Adding/changing a sensor — the five places

A **protocol sensor** touches: `sensors_config.py`; **all five** `translations/*.py`; enum + setter +
member in `daikin_rotex_solaris.h`; `dump_config()` + `publish_values_()` + `invalidate_all_sensors_()`
in the `.cpp`; `solaris-template.yaml` (+ dashboard YAML/SVG if visualised). Variations:

- **Static** diagnostics (`git_hash`): bypass the list — declared in `__init__.py` `CONFIG_SCHEMA`,
  published once in `setup()`, so `dump_config()` only (no per-cycle publish, not invalidated).
- **Computed** (`solaris_tz`, `solaris_deltat`): same five places but **no enum step**; computed inline
  in `publish_values_()`. `solaris_tz` also needs a public reset method + midnight `time: on_time:`
  trigger, and publishes NAN (without resetting the accumulator) in `invalidate_all_sensors_()`.

## HA dashboard (`ha-dashboard/`)

Files: `solaris-rps-dashboard.svg` (ha-floorplan image), `.css` (classes), `.yaml`
(`custom:floorplan-card` rules), `convert-svg.py`. Deploy manually: copy `.svg` + `.css` into
`/config/www/floorplans/solaris-rps/`; the card's `image:`/`stylesheet:` paths must match.

**SVG id hooks** (draw.io cell ids — a typo silently no-ops, no HA error):
- `<key>_txt` static label · `<key>_val` value (via `floorplan.text_set`) · `cell-<id>` wrapping
  group/shape to show/hide or style.

**Rule conventions:**
- Offline guard on every value rule:
  `'${entity.state === "unknown" || entity.state === "unavailable" ? "N/A" : …}'`.
- Formatting matches firmware precision: `parseInt + " °C"`, `toFixed(1) + " l/min"`,
  `toFixed(2) + " kW"`, `parseInt + "%"`; binaries render localized `ein`/`aus`.
- Colours: temps/flow `txt-blue`, collector `txt-red`, pump `txt-green`, power/manual `txt-yellow`,
  unavailable `txt-grey`. Static labels `non-clickable`; indicators toggle `visible`/`hidden`.
- Pipe flow animated via `flow-animation visible` on `cell-wasser_hoch`/`cell-wasser_runter` while
  circ pump `> 0`.
- `tz_val` binds to `sensor.esp_rotex_solaris_rps3_tagesspeicherzuwachstemperatur` (firmware sensor,
  not the old `input_number`).
- **Entity ids are the German default-language names** (e.g.
  `sensor.esp_rotex_solaris_rps3_kollektortemperatur`). Changing device name, `friendly_name`, or
  `language:` changes them — update the dashboard YAML in lockstep.

**Regenerate the SVG** only after editing `drawio/solaris-dashboard.drawio`: export with the draw.io
`svgdata` plugin on (so `data-cell-id` survives), then run `convert-svg.py`.

## Cross-cutting rules

- **Translations are all-or-nothing:** `de en fr it es` each export `SENSOR_NAMES_<LANG>` +
  `ERROR_CODES_<LANG>` with identical keys; register new languages in both maps in `translations.py`.
  `de` is `DEFAULT_LANGUAGE` + fallback.
- **Docs are bilingual:** update `README.md` (German) **and** `README.en.md` (both repo root and
  `ha-dashboard/`), keeping the de/en badge header.
- **RPS4** (no booster pump): comment out the `solaris_p2` block in YAML and the `p2_*` dashboard rules;
  keep those documented comment blocks intact.
- Keep the `# ===== SECTION =====` banner-comment style in Python, C++, and YAML.
