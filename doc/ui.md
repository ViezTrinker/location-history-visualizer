# Benutzeroberfläche

Die UI ist Qt 6 Widgets. Verbindungen laufen über Member-Slots, ohne Lambdas.

## MainWindow

[`src/main_window.h`](../src/main_window.h) hält zwei Punktlisten:

- `_allPoints` — Ergebnis von `LoadFromFile`
- `_filteredPoints` — nach `ApplyFilter`

Layout: linke Leiste (~280 px) + `MapWidget`.

| Bereich | Steuerung |
| --- | --- |
| Datei | Button / Menü Datei → Öffnen, `QFileDialog` für `*.json` |
| Datum | `QDateEdit` von/bis, nach dem Laden auf Min/Max der Daten gesetzt |
| Wochentag | sieben Checkboxen, Bits in `weekdayMask` |
| Uhrzeit | `QTimeEdit` 00:00–23:59 |
| Darstellung | ComboBox `DisplayMode` |
| Skalierung | `QSlider`, nur bei Heatmap und Blur aktiv, Faktor ×1..×100 logarithmisch |
| Punktinfo | Wann, Latitude, Longitude |

`OnOpenClicked` setzt Wait-Cursor, lädt synchron, zeigt `LoadResult` als MessageBox bei Fehler, zentriert die Karte auf die Punkte.

Menü **Hilfe → About** öffnet [`src/about_dialog.h`](../src/about_dialog.h): Version, Datum, Link ViezTrinker, Repo-Platzhalter aus [`src/version.h`](../src/version.h). Links sind `QLabel` mit `setOpenExternalLinks`.

## Einstieg

[`src/main.cpp`](../src/main.cpp) setzt `applicationName` / `organizationName` aus denselben Version-Strings. Davon hängt der Tile-Cache-Pfad über `QStandardPaths::AppLocalDataLocation` ab.

## Zuständigkeiten

```mermaid
flowchart LR
  ui[MainWindow Filter und Datei]
  coreFilter[ApplyFilter]
  map[MapWidget]
  ui -->|"_allPoints plus FilterSettings"| coreFilter
  coreFilter -->|"_filteredPoints"| map
  ui -->|DisplayMode HeatScale| map
  map -->|PointClicked| ui
```

`MainWindow` orchestriert. Zeichnen, Zoom und Kacheln bleiben in `MapWidget`. Parse/Filter bleiben in der Kernbibliothek.
