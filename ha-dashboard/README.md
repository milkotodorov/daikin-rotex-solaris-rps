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
