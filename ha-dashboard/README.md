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
- **Tz** (Tagesspeicherzuwachstemperatur) — Speichertemperaturanstieg, der ausschließlich auf die solare Erwärmung zurückzuführen ist, in °C. Wird um Mitternacht auf 0 zurückgesetzt.

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

**Ergebnis-Entität:** `sensor.rotex_solaris_rps3_leistung_total`

#### Schritt 2 — Verbrauchszähler (täglicher Reset)

**Einstellungen → Geräte & Dienste → Helfer → Helfer erstellen → Verbrauchszähler**:

| Feld | Wert |
|------|------|
| Name | `ROTEX Solaris RPS Leistung Tagesertrag` |
| Eingangssensor | `sensor.rotex_solaris_rps3_leistung_total` |
| Rückstellzyklus | Täglich |
| Genauigkeit | 2 |

**Ergebnis-Entität:** `sensor.rotex_solaris_rps3_leistung_tagesertrag`

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

**Ergebnis-Entität:** `sensor.rotex_solaris_rps3_durchfluss_total`

#### Schritt 2 — Verbrauchszähler (täglicher Reset)

**Einstellungen → Geräte & Dienste → Helfer → Helfer erstellen → Verbrauchszähler**:

| Feld | Wert |
|------|------|
| Name | `ROTEX Solaris RPS Durchfluss Tagesertrag` |
| Eingangssensor | `sensor.rotex_solaris_rps3_durchfluss_total` |
| Rückstellzyklus | Täglich |
| Genauigkeit | 0 |

**Ergebnis-Entität:** `sensor.rotex_solaris_rps3_durchfluss_tagesertrag`

> Das Dashboard zeigt diesen Wert im SVG-Element `df_day_val` (nicht `vt_val`) — die Beschriftung „Vt" ist statischer Text.

---

### Tz — Tagesspeicherzuwachstemperatur

Tz wird direkt in der Firmware berechnet und als `sensor.esp_rotex_solaris_rps3_tagesspeicherzuwachstemperatur` bereitgestellt (der Entitätsname richtet sich nach der konfigurierten `language:`). Es sind keine HA-Helfer oder Automationen erforderlich.

Die Firmware summiert ganzzahlige Ts-Anstiege während des Solarbetriebs (P > 0,01 kW) und setzt den Wert um Mitternacht auf 0 zurück. Rauschen und Warmwasserentnahmen werden mit 90-s-Haltezeiten gefiltert; kurze Pumpenzykluslücken werden mit einer 3-Minuten-Haltezeit im Aus-Betrieb behandelt. Die vollständige Logikbeschreibung findet sich in `AGENTS.md`.

> [!NOTE]
> **ESP-Neustart:** Der Akkumulator liegt im RAM, daher setzt ein ESP-Neustart den tagesaktuellen Tz-Wert auf 0 zurück. Neustarts sind selten, und das Design verzichtet bewusst auf eine NVS-Flash-Persistenz, um Flash-Verschleiß zu vermeiden. Vergangene Tageswerte bleiben stets über HA-Langzeitstatistiken erhalten (`state_class: total_increasing`).
>
> Um den tagesaktuellen Wert nach einem Neustart wiederherzustellen, die ESPHome-Aktion **`esphome.esp_rotex_solaris_rps3_set_tz`** aufrufen (HA → Entwicklerwerkzeuge → Aktionen). Den zuletzt bekannten Tz-Wert als `value` (Ganzzahl, 0–100) übergeben. Der ESP setzt seinen internen Akkumulator auf diesen Wert und zählt von dort weiter – es wird keine HA-Entität erstellt und kein Flash beschrieben.
