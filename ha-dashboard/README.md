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

Tz zählt einfach, um wie viele Grad die Speichertemperatur (Ts) heute gestiegen ist, während die Solarpumpe lief. Immer wenn Ts steigt und oben bleibt, wird der Anstieg zum Zähler addiert. Rückgänge (durch Warmwasserentnahme oder Sensorrauschen) werden ignoriert — nach einem Rückgang wird ab der neuen niedrigeren Temperatur einfach weitergezählt. Der Zähler wird um Mitternacht zurückgesetzt.

**Logik:**
- `last_ts` speichert die Referenztemperatur, ab der der nächste Anstieg gemessen wird.
- Während der Solarbetrieb **aus** ist (P ≤ 0,01 kW): `last_ts` an den aktuellen Ts-Wert koppeln. Dadurch entspricht `last_ts` im Moment des Solarstarts bereits der aktuellen Temperatur, sodass nur echte Erwärmung nach dem Start gezählt wird — nie ein Anstieg über Nacht oder durch den Nachheizkessel.
- Wenn Ts über `last_ts` steigt **und 60 s dort bleibt** während P > 0,01 kW: `(Ts − last_ts)` zu Tz addieren, dann `last_ts = Ts` setzen. Die 60-s-Haltezeit filtert vorübergehendes ±1 °C Sensorrauschen — ein einzelner Fehlwert kehrt vor Ablauf der Haltezeit zurück, während echte solare Erwärmung bestehen bleibt. Als Sicherheitsnetz: übersteigt der scheinbare Anstieg 10 °C (für echte Erwärmung innerhalb eines 60-s-Fensters unmöglich — passiert nur, wenn `last_ts` durch einen Neustart veraltet ist), wird `last_ts` neu gesetzt statt gezählt.
- Wenn Ts unter `last_ts` fällt **und 60 s dort bleibt**: `last_ts` auf den aktuellen Ts-Wert senken. Das addiert keinen Gewinn, sodass eine Warmwasserentnahme nie gegen Sie zählt — der nächste echte Anstieg wird einfach ab dem niedrigeren Wert gezählt. Die 60-s-Haltezeit verhindert außerdem, dass `last_ts` vorübergehenden ±1 °C Rausch-Einbrüchen hinterherläuft: ein Ausschlag, der innerhalb von 60 s zurückkehrt, lässt `last_ts` unverändert, sodass eine schnelle ±1 °C Oszillation auf beiden Flanken vollständig ignoriert wird und Tz nie aufbläht.
- Um Mitternacht: Tz auf 0 zurücksetzen und `last_ts` auf den aktuellen Ts-Wert setzen.

Da Ts nur in ganzen Grad gemeldet wird, ist Tz immer eine ganze Zahl. Alle Helfer sind `input_number`-Entitäten, die Home Assistant über Neustarts hinweg wiederherstellt — ein Neustart von HA oder des ESPHome-Geräts mitten am Tag setzt die Zählung dort fort, wo sie unterbrochen wurde (siehe Hinweise nach den Automationen).

#### Schritt 1 — Helfer

Gehen Sie zu **Einstellungen → Geräte & Dienste → Helfer → Helfer erstellen → Zahl** und erstellen Sie zwei Helfer:

| Name | Min | Max | Schritt | Einheit | Modus |
|------|-----|-----|---------|---------|-------|
| `ROTEX Solaris RPS Temperaturzuwachs` | 0 | 60 | 1 | °C | Eingabefeld |
| `ROTEX Solaris RPS Tz Last Ts` | 0 | 85 | 1 | °C | Eingabefeld |

#### Schritt 2 — Automationen

Importieren Sie jede Automation einzeln über **Einstellungen → Automationen → ⋮ → YAML importieren**.

```yaml
alias: "ROTEX Solaris RPS Tz - baseline while off"
description: "While solar is off (P ≤ 0.01 kW), keep last_ts glued to the current Ts, so counting resumes from the current temperature the moment solar starts."
mode: restart
triggers:
  - trigger: state
    entity_id: sensor.esp_rotex_solaris_rps3_speichertemperatur
  - trigger: numeric_state
    entity_id: sensor.esp_rotex_solaris_rps3_leistung
    below: 0.01
conditions:
  - condition: template
    value_template: >
      {{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') not in ['unknown', 'unavailable'] }}
  - condition: numeric_state
    entity_id: sensor.esp_rotex_solaris_rps3_leistung
    below: 0.01
  - condition: template
    value_template: >
      {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float %}
      {% set last = states('input_number.rotex_solaris_rps_tz_last_ts') | float %}
      {{ ts != last }}
actions:
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_last_ts
    data:
      value: "{{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float }}"
```

```yaml
alias: "ROTEX Solaris RPS Tz - count rise"
description: "While solar runs (P > 0.01 kW), when Ts rises above last_ts and holds for 60s, add the rise to Tz. The 60s hold rejects transient ±1 °C sensor noise."
mode: restart
triggers:
  - trigger: template
    value_template: >
      {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float(0) %}
      {% set last = states('input_number.rotex_solaris_rps_tz_last_ts') | float(0) %}
      {{ ts > last }}
    for: "00:01:00"
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
      {% set last = states('input_number.rotex_solaris_rps_tz_last_ts') | float %}
      {{ ts > last }}
actions:
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_temperaturzuwachs
    data:
      value: >
        {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float %}
        {% set last = states('input_number.rotex_solaris_rps_tz_last_ts') | float %}
        {% set tz = states('input_number.rotex_solaris_rps_temperaturzuwachs') | float %}
        {% set rise = ts - last %}
        {{ (tz + rise) | round(0) | int if 0 < rise <= 10 else tz | round(0) | int }}
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_last_ts
    data:
      value: "{{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float }}"
```

```yaml
alias: "ROTEX Solaris RPS Tz - track drop"
description: "When Ts drops below last_ts and stays down for 60s, lower last_ts to the current Ts so the next rise is counted from there. Adds no gain. The 60s hold stops last_ts from chasing transient ±1 °C noise dips."
mode: restart
triggers:
  - trigger: template
    value_template: >
      {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float(999) %}
      {% set last = states('input_number.rotex_solaris_rps_tz_last_ts') | float(0) %}
      {{ ts < last }}
    for: "00:01:00"
conditions:
  - condition: template
    value_template: >
      {{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') not in ['unknown', 'unavailable'] }}
  - condition: template
    value_template: >
      {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float(999) %}
      {% set last = states('input_number.rotex_solaris_rps_tz_last_ts') | float(0) %}
      {{ ts < last }}
actions:
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_last_ts
    data:
      value: "{{ states('sensor.esp_rotex_solaris_rps3_speichertemperatur') | float }}"
```

```yaml
alias: "ROTEX Solaris RPS Tz - midnight reset"
description: "At 00:00 reset Tz to 0 and re-baseline last_ts to the current Ts for the new day"
triggers:
  - trigger: time
    at: "00:00:00"
conditions: []
actions:
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_temperaturzuwachs
    data:
      value: 0
  - action: input_number.set_value
    target:
      entity_id: input_number.rotex_solaris_rps_tz_last_ts
    data:
      value: >
        {% set ts = states('sensor.esp_rotex_solaris_rps3_speichertemperatur') %}
        {{ ts | float if ts not in ['unknown', 'unavailable'] else states('input_number.rotex_solaris_rps_tz_last_ts') | float }}
```

**Ergebnis-Entität:** `input_number.rotex_solaris_rps_temperaturzuwachs`

> [!NOTE]
> **Verhalten bei Neustart.** Alle vier Zustandswerte liegen in `input_number`-Helfern, die Home Assistant über Neustarts hinweg wiederherstellt, sodass ein Neustart mitten am Tag die Zählung dort fortsetzt, wo sie aufgehört hat.
> - **HA-Neustart bei laufendem Solarbetrieb:** Helfer werden wiederhergestellt; höchstens ~1 °C kann während der wenigen Sekunden Ausfall verpasst werden. Falls ein Helfer je auf einen veralteten/niedrigen Wert wiederhergestellt wird, setzt das 10-°C-Sicherheitsnetz von „count rise" neu, statt einen Phantom-Sprung zu zählen.
> - **Neustart des ESPHome-Geräts:** Ts und P lesen kurz `unavailable`. Jede Automation prüft auf `unavailable` und überspringt es, und die 60-s-Haltezeiten überdauern die wenige Sekunden dauernde Wiederverbindung — es entsteht keine fehlerhafte Zählung.
> - **Erstinstallation / Helfer nie gesetzt:** Helfer anlegen, dann setzt die Automation „baseline while off" `last_ts` beim nächsten Ts-Wert bei ausgeschaltetem Solarbetrieb auf den aktuellen Ts (oder das 10-°C-Sicherheitsnetz fängt es ab, falls Solarbetrieb bereits läuft). In beiden Fällen wird kein Phantom-Gewinn gezählt.
