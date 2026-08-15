# Benutzeroberfläche

Die UI ist Qt 6 Widgets. Verbindungen laufen über Member-Slots, ohne Lambdas.

## MainWindow

[`src/main_window.h`](../src/main_window.h) hält zwei Punktlisten:

- `_allPoints` — Ergebnis von `LoadFromFile`
- `_filteredPoints` — nach `ApplyFilter`

Menüleiste: **File | Settings | Help**. Layout darunter: linke Leiste (~280 px) + `MapWidget` + Zoom-Leiste rechts neben der Karte + Zeit-Scrubber darunter.

| Bereich | Steuerung |
| --- | --- |
| File | Menu and sidebar button → Open, `QFileDialog` for `*.json`. Last path in `QSettings` (`lastJsonPath`). Exit closes the window. |
| Settings | **Language** submenu: exclusive checkable actions with native names (English, Deutsch, Español, Français, Русский, العربية, Italiano, Türkçe, Nederlands, Português, Polski). Stored in `QSettings` (`language`). Arabic sets `Qt::RightToLeft`. **Theme** submenu: Dark (default) or Light. Stored in `QSettings` (`theme`). Fusion palettes, independent of the Windows color mode. **Map display...** opens [`src/map_display_dialog.h`](../src/map_display_dialog.h): point radius 1–16 px (default 4) and maximum drawn points 1000–1 000 000 (default 20 000). Stored in `QSettings` (`pointRadiusPx`, `drawnPointLimit`). |
| Help | About |
| Date | `QDateEdit` from/to, set to min/max of the data after load |
| Weekday | seven checkboxes, bits in `weekdayMask` |
| Time of day | `QTimeEdit` 00:00–23:59 |
| Display | ComboBox `DisplayMode`: Points, Cluster, or Story |
| Zoom | `+`, vertical `QSlider` (zoom 2..19), `-` — beside the map |
| Story | Visible only in Story mode: start-day picker, Play/Pause, scrubber — Play continues through later days as red points |
| Point info | When, Until, Duration, Latitude, Longitude |
| Counts | At the bottom of the sidebar. **In file**: `_allPoints.size()`. **Visible**: points that current filters (and Story cutoff) allow, capped by the drawn-point limit except in Cluster mode |

Source UI strings are English. Translations live in [`translations/`](../translations/) (`.ts` → `.qm` via Qt LinguistTools). Changing the language updates the window immediately.

`OnOpenClicked` sets a wait cursor, loads synchronously, shows `LoadResult` as a message box on error, and centers the map on the densest cell (`CenterOnPoints`).

Menu **Help → About** opens [`src/about_dialog.h`](../src/about_dialog.h): version `1.0.0.R`, date, ViezTrinker link, repo [location-history-visualizer](https://github.com/ViezTrinker/location-history-visualizer). Strings and URLs come from [`src/version.h`](../src/version.h). Links are `QLabel` with `setOpenExternalLinks`.

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
  ui -->|DisplayMode UntilTime Zoom| map
  map -->|PointClicked ZoomChanged| ui
```

`MainWindow` orchestriert. Zeichnen und Kacheln bleiben in `MapWidget`. Zoom-Logik (`SetZoomAround`, Mausrad, Doppelklick) ebenfalls dort; die Zoom-Leiste gehört zu `MainWindow` und folgt über `ZoomChanged`. Parse/Filter bleiben in der Kernbibliothek.
