[![de](https://img.shields.io/badge/lang-de-red.svg)](README.md)
[![en](https://img.shields.io/badge/lang-en-blue.svg)](README.en.md)

# Solaris RPS3/4 Dashboard für Home Assistant

Dieses Dashboard bietet eine visuelle Darstellung eines ROTEX Solaris RPS3/4-Systems in Home Assistant unter Verwendung von [ha-floorplan](https://github.com/ExperienceLovelace/ha-floorplan).

Das Dashboard besteht aus:

- `solaris-rps-dashboard.yaml` – `ha-floorplan` Konfiguration (Entitäten, Aktionen, Stile).
- `solaris-rps-dashboard.css` – CSS für die Gestaltung des SVG und der Zustände.
- `solaris-rps-dashboard.svg` – SVG Dashboard Bild.
- `convert-svg.py` – Hilfsskript zum Konvertieren einer draw.io SVG Datei in eine mit ha-floorplan kompatible SVG Datei.

> [!NOTE]
> Die SVG Datei in diesem Repository ist **bereits konvertiert** und kann direkt verwendet werden.

### Voraussetzungen

- Home Assistant mit Dateizugriff auf `/config`.
- Installiertes [HACS](https://www.hacs.xyz).
- Installiertes [ha-floorplan](https://github.com/ExperienceLovelace/ha-floorplan) (vorzugsweise über HACS). Lesen Sie [Adding ha-floorplan to Home Assistant](https://experiencelovelace.github.io/ha-floorplan/docs/quick-start/#adding-ha-floorplan-to-home-assistant) zur Installation von `ha-floorplan`.
- Optional für die SVG Konvertierung (wenn Sie die bereits vorhandene SVG Datei ändern möchten): Python 3.x zum Ausführen von `convert-svg.py` (getestet mit Python 3.12).

### Kopieren die Dateien in Home Assistant-Konfiguration:

- Erstellen Sie einen Ordner unter `/config/www`, in den Sie die `solaris-rps-dashboard` Dateien hochladen können. Zum Beispiel `floorplans/solaris-rps`.
- Laden Sie die Dateien `ha-dashboard/solaris-rps-dashboard.css` und `ha-dashboard/solaris-rps-dashboard.svg` in den erstellten Ordner `/config/www/floorplans/solaris-rps` hoch.

### Anpassen der YAML-Konfiguration

Sie können den Inhalt von `ha-dashboard/solaris-rps-dashboard.yaml` an Ihre Bedürfnisse anpassen, z. B. die Textbeschriftungen ändern und/oder das Dashboard für den Solaris RPS3/4-Typ anpassen.

### Konfigurieren der Karte „ha-floorplan“

Sie können das Dashboard auf zwei Arten erstellen:

- In einer neuen `Ansicht`:

Gehen Sie zu `Ansicht hinzufügen` -> `In YAML bearbeiten` und fügen Sie dort den Inhalt von `ha-dashboard/solaris-rps-dashboard.yaml` ein.

- In einer bestehenden `Ansicht`:

  Klicken Sie in Ihrer bestehenden `Ansicht` auf `Karte hinzufügen` -> `Manuell` und fügen Sie den Inhalt von `ha-dashboard/solaris-rps-dashboard.yaml` aus `type: custom:floorplan-card` unten ein.

> [!TIP]
> Optional können Sie `full_height: true` entfernen, wenn Sie keine Vollbildansicht wünschen.

### Optionale SVG Konvertierung

Wenn Sie das vorhandene `draw.io` Diagramm Dashboard ändern, können Sie das Skript `convert-svg.py` verwenden, um die von `draw.io` generierten `foreignObject`-Tags durch native SVG Textelemente zu ersetzen, damit es mit `ha-floorplan` funktioniert. Zusätzlich müssen Sie das `svgdata` [Plugin für `draw.io`](https://www.drawio.com/doc/faq/plugins) installieren, um die IDs in der exportierten SVG Datei zu speichern.


```shell
usage: convert-svg.py [-h] [-o OUTPUT] [input]

draw.io → ha-floorplan SVG converter

positional arguments:
  input                 Input SVG

options:
  -h, --help            show this help message and exit
  -o OUTPUT, --output OUTPUT
                        Output SVG (optional)
```

---

## Benutzerdefinierte Sensoren erstellen

Das Dashboard zeigt drei Elemente für abgeleitete Tagesstatistiken, die der Controller nicht direkt überträgt:

- **Te** (Tagesertrag) — gesamter solarer Energieertrag heute, in kWh. Wird um Mitternacht auf 0 zurückgesetzt.
- **Vt** (Volumen Tag) — gesamtes heute gepumptes Wasservolumen, in Litern. Wird um Mitternacht auf 0 zurückgesetzt.
- **Tz** (Temperaturzuwachs) — Speichertemperaturanstieg, der ausschließlich auf die solare Erwärmung zurückzuführen ist, in °C. Wird um Mitternacht auf 0 zurückgesetzt.

Ersetzen Sie `esp_rotex_solaris_rps3` überall durch Ihren tatsächlichen ESPHome-Gerätenamen. HA-erstellte Helfer-Entitäts-IDs können je nach Gerätezuweisung auch ein Bereichspräfix enthalten — prüfen Sie die tatsächlichen Entitäts-IDs unter **Entwicklerwerkzeuge → Zustände** und aktualisieren Sie `solaris-rps-dashboard.yaml` entsprechend.

> [!NOTE]
> HA hängt den Gerätenamen automatisch vor den Helfernamen und kann zudem einen Bereichsnamen (`Area_`) der Entitäts-ID voranstellen. Die in dieser Anleitung angegebenen Entitäts-IDs sind daher nur Beispiele. Prüfen Sie nach dem Erstellen jedes Helfers die tatsächliche Entitäts-ID unter **Entwicklerwerkzeuge → Zustände** und verwenden Sie genau diese ID in `solaris-rps-dashboard.yaml`.

---

### Te — Täglicher Energieertrag

Die tägliche akkumulierte Energie in kWh aus einem Leistungssensor (kW) mit automatischem Mitternachts-Reset erfordert zwei Helfer: einen **Riemann-Summen-Integralsensor** (wandelt momentane kW in kumulierte kWh um) und einen **Verbrauchszähler** (setzt um Mitternacht zurück).

#### Schritt 1 — Riemann-Summen-Integralsensor

**Einstellungen → Geräte & Dienste → Helfer → Helfer erstellen → Integralsensor**:

| Feld | Wert |
|------|------|
| Name | `ROTEX Solaris RPS Leistung Total` |
| Eingangssensor | `sensor.esp_rotex_solaris_rps3_leistung` |
| Integrationsmethode | Linke Riemann-Summe |
| Metrisches Präfix | Keines |
| Integrationszeit | Stunden |
| Genauigkeit | 2 |

> Die linksseitige Riemann-Summe verhindert künstliche Spitzen durch schwankende Solarleistungswerte.

**Ergebnis-Entität:** `sensor.rotex_solaris_rps_leistung_total`

#### Schritt 2 — Verbrauchszähler (täglicher Reset)

**Einstellungen → Geräte & Dienste → Helfer → Helfer erstellen → Verbrauchszähler**:

| Feld | Wert |
|------|------|
| Name | `ROTEX Solaris RPS Leistung Tagesertrag` |
| Eingangssensor | `sensor.rotex_solaris_rps_leistung_total` |
| Rückstellzyklus | Täglich |
| Genauigkeit | 2 |

**Ergebnis-Entität:** `sensor.rotex_solaris_rps_leistung_tagesertrag`

---

### Vt — Tägliches Wasservolumen

Erfasst die gesamten pro Tag gepumpten Liter. Durchfluss wird in `l/min` gemessen, daher muss das Integral **Minuten** als Zeitbasis verwenden, damit das Ergebnis direkt in Litern vorliegt.

#### Schritt 1 — Riemann-Summen-Integralsensor

**Einstellungen → Geräte & Dienste → Helfer → Helfer erstellen → Integralsensor**:

| Feld | Wert |
|------|------|
| Name | `ROTEX Solaris RPS Durchfluss Total` |
| Eingangssensor | `sensor.esp_rotex_solaris_rps3_durchfluss` |
| Integrationsmethode | Linke Riemann-Summe |
| Metrisches Präfix | Keines |
| Integrationszeit | Minuten |
| Genauigkeit | 1 |

**Ergebnis-Entität:** `sensor.rotex_solaris_rps_durchfluss_total`

#### Schritt 2 — Verbrauchszähler (täglicher Reset)

**Einstellungen → Geräte & Dienste → Helfer → Helfer erstellen → Verbrauchszähler**:

| Feld | Wert |
|------|------|
| Name | `ROTEX Solaris RPS Durchfluss Tagesertrag` |
| Eingangssensor | `sensor.rotex_solaris_rps_durchfluss_total` |
| Rückstellzyklus | Täglich |
| Genauigkeit | 0 |

**Ergebnis-Entität:** `sensor.rotex_solaris_rps_durchfluss_tagesertrag`

> Das Dashboard zeigt diesen Wert im SVG-Element `df_day_val` (nicht `vt_val`) — die Beschriftung „Vt" ist statischer Text.

---

### Tz — Solarer Temperaturzuwachs im Speicher

Tz misst nur den durch den Solarkollektor verursachten Temperaturanstieg. Um Wolkenlücken korrekt zu behandeln (P fällt für Minuten oder Stunden auf 0 und nimmt dann wieder zu), wird der Anstieg in **Segmenten** verfolgt: Jedes Mal, wenn der Solarertrag nach einer Pause wieder startet, wird eine neue Segment-Baseline erfasst, und der Gewinn aus allen abgeschlossenen Segmenten wird in einem Akkumulator gespeichert.

**Logik:**
- Wenn P von 0 auf >0 wechselt: aktuellen Ts als `segment_start` speichern.
- Während P > 0: `Tz = akkumuliert + max(Ts_aktuell − segment_start, 0)`.
- Wenn P von >0 auf 0 wechselt: `max(Ts_aktuell − segment_start, 0)` zu `akkumuliert` addieren; `segment_start` zurücksetzen.
- Wenn Ts **um ≥ 2 °C sinkt** während P > 0 (z.B. Warmwasserentnahme): aktuellen Segment-Gewinn in `akkumuliert` einschließen, `segment_start` auf den neuen niedrigeren Ts-Wert setzen. So wird die anschließende solare Wiedererwärmung ab dem niedrigeren Niveau vollständig angerechnet.
- Um Mitternacht: alle drei Helfer auf 0 zurücksetzen.

#### Schritt 1 — Helfer

Gehen Sie zu **Einstellungen → Geräte & Dienste → Helfer → Helfer erstellen → Zahl** und erstellen Sie drei Helfer:

| Name | Min | Max | Schritt | Einheit | Modus |
|------|-----|-----|---------|---------|-------|
| `ROTEX Solaris RPS Tz Segment Start` | 0 | 85 | 1 | °C | Eingabefeld |
| `ROTEX Solaris RPS Tz Accumulated` | 0 | 60 | 0.1 | °C | Eingabefeld |
| `ROTEX Solaris RPS Temperaturzuwachs` | 0 | 60 | 1 | °C | Eingabefeld |

#### Schritt 2 — Automationen

Importieren Sie jede Automation einzeln über **Einstellungen → Automationen → ⋮ → YAML importieren**.

```yaml
alias: "ROTEX Solaris RPS Tz - solar segment start"
description: "When P goes from 0 to ≥ 0.01 kW, capture current Ts as the segment baseline"
triggers:
  - trigger: numeric_state
    entity_id: sensor.esp_rotex_solaris_rps3_leistung
    above: 0.01
conditions:
  - condition: numeric_state
    entity_id: input_number.rotex_solaris_rps_tz_segment_start
    below: 0.01
  - condition: template
    value_template: >
      {{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') not in ['unknown', 'unavailable'] }}
actions:
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_segment_start
    data:
      value: "{{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float }}"
```

```yaml
alias: "ROTEX Solaris RPS Tz - solar segment end"
description: "When P drops to 0, add this segment's Ts gain to the accumulator and clear the baseline"
triggers:
  - trigger: numeric_state
    entity_id: sensor.esp_rotex_solaris_rps3_leistung
    below: 0.01
    for: "00:00:10"
conditions:
  - condition: numeric_state
    entity_id: input_number.rotex_solaris_rps_tz_segment_start
    above: 0
  - condition: template
    value_template: >
      {{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') not in ['unknown', 'unavailable'] }}
actions:
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_accumulated
    data:
      value: >
        {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float %}
        {% set seg = states('input_number.rotex_solaris_rps_tz_segment_start') | float %}
        {% set acc = states('input_number.rotex_solaris_rps_tz_accumulated') | float %}
        {{ (acc + [ts - seg, 0] | max) | round(1) }}
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_segment_start
    data:
      value: 0
```

```yaml
alias: "ROTEX Solaris RPS Tz - Ts drop rebaseline"
description: "Wenn Ts um ≥ 2 °C sinkt während P > 0 (z.B. Warmwasserentnahme), Segment-Gewinn einschließen und auf neuen Ts-Wert setzen"
triggers:
  - trigger: state
    entity_id: sensor.esp_rotex_solaris_rps3_speichertemperatur
conditions:
  - condition: template
    value_template: >
      {{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') not in ['unknown', 'unavailable'] }}
  - condition: numeric_state
    entity_id: sensor.esp_rotex_solaris_rps3_leistung
    above: 0.01
  - condition: template
    value_template: >
      {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float %}
      {% set seg = states('input_number.rotex_solaris_rps_tz_segment_start') | float %}
      {{ seg > 0 and (seg - ts) >= 2 }}
actions:
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_accumulated
    data:
      value: >
        {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float %}
        {% set seg = states('input_number.rotex_solaris_rps_tz_segment_start') | float %}
        {% set acc = states('input_number.rotex_solaris_rps_tz_accumulated') | float %}
        {{ (acc + [ts - seg, 0] | max) | round(1) }}
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_segment_start
    data:
      value: "{{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float }}"
```

```yaml
alias: "ROTEX Solaris RPS Tz - live update"
description: "Whenever Ts changes or P turns on, recompute Tz as accumulated gain plus current segment gain"
triggers:
  - trigger: state
    entity_id: sensor.esp_rotex_solaris_rps3_speichertemperatur
  - trigger: numeric_state
    entity_id: sensor.esp_rotex_solaris_rps3_leistung
    above: 0.01
conditions:
  - condition: template
    value_template: >
      {{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') not in ['unknown', 'unavailable'] }}
  - condition: numeric_state
    entity_id: sensor.esp_rotex_solaris_rps3_leistung
    above: 0.01
  - condition: numeric_state
    entity_id: input_number.rotex_solaris_rps_tz_segment_start
    above: 0
actions:
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_temperaturzuwachs
    data:
      value: >
        {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float %}
        {% set seg = states('input_number.rotex_solaris_rps_tz_segment_start') | float %}
        {% set acc = states('input_number.rotex_solaris_rps_tz_accumulated') | float %}
        {{ (acc + [ts - seg, 0] | max) | round(0) | int }}
```

```yaml
alias: "ROTEX Solaris RPS Tz - midnight reset"
description: "At 00:00 reset all three helpers to 0 for the new day"
triggers:
  - trigger: time
    at: "00:00:00"
conditions: []
actions:
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_segment_start
    data:
      value: 0
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_accumulated
    data:
      value: 0
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_temperaturzuwachs
    data:
      value: 0
```

**Ergebnis-Entität:** `input_number.rotex_solaris_rps_temperaturzuwachs`

> [!NOTE]
> `segment_start = 0` ist der Sentinel-Wert für „kein aktives Segment". Dies funktioniert, weil Ts = 0 °C im Solarbetrieb physikalisch unmöglich ist. Wenn Ihre Anlage in einem extremen Klima betrieben wird, in dem Ts tatsächlich 0 °C erreichen könnte, ändern Sie den Sentinel-Wert auf −1 und aktualisieren Sie **alle** folgenden Stellen: die Bedingung `above: 0` in der Segment-End-Automation, die Prüfung `seg > 0` in der Rebaseline-Template-Bedingung, den Minimalwert des Helfers `ROTEX Solaris RPS Tz Segment Start` von 0 → −1 sowie alle vier Reset-Werte (in der Mitternachts-Reset- und der Segment-End-Automation) von `0` → `-1`.
