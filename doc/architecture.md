# Architektur

Der Visualizer ist eine native Windows-Desktopanwendung in C++20. Die GUI sitzt auf Qt 6, die Karte besteht aus OpenStreetMap-Rasterkacheln plus selbst gezeichnetem Overlay.

Alles fachliche liegt im Namespace `LocationHistory`. `src/main.cpp` startet nur `QApplication` und `MainWindow`.

## Schichten

CMake teilt den Code in drei Targets. Die Kernbibliothek kennt Qt nicht. Die Tests linken nur den Kern.

```mermaid
flowchart TB
  subgraph app [location_history_visualizer]
    mainCpp[main.cpp]
    mainWindow[MainWindow]
    mapWidget[MapWidget]
    aboutDialog[AboutDialog]
    appLanguage[app_language]
    tileCache[TileCache]
    tileDownloader[TileDownloader]
  end

  subgraph core [location_history_core]
    jsonLoader[json_loader]
    filter[location_filter]
    civilTime[civil_time]
    tileMath[tile_math]
    clusterer[clusterer]
    heatmap[heatmap_renderer]
    mapFocus[map_focus]
    dataModel[location_point / location_data]
  end

  subgraph tests [location_history_visualizer_tests]
    gtest[gtest]
  end

  mainCpp --> mainWindow
  mainCpp --> appLanguage
  mainWindow --> mapWidget
  mainWindow --> aboutDialog
  mainWindow --> appLanguage
  mainWindow --> jsonLoader
  mainWindow --> filter
  mapWidget --> tileCache
  mapWidget --> tileDownloader
  mapWidget --> tileMath
  mapWidget --> clusterer
  mapWidget --> heatmap
  mapWidget --> mapFocus
  jsonLoader --> dataModel
  jsonLoader --> civilTime
  filter --> dataModel
  filter --> civilTime
  tests --> core
  tests --> gtest
```

| Target | Rolle | Qt |
| --- | --- | --- |
| `location_history_core` | Statische Bibliothek: Parse, Filter, Projektion, Visualisierungsmathe | nein |
| `location_history_visualizer` | `.exe`: Fenster, Kacheln, Zeichnen | Widgets + Network |
| `location_history_visualizer_tests` | gtest gegen den Kern | nein |

## Laufzeitfluss

```mermaid
sequenceDiagram
  participant User
  participant MainWindow
  participant JsonLoader
  participant Filter
  participant MapWidget
  participant TileDownloader

  User->>MainWindow: JSON öffnen
  MainWindow->>JsonLoader: LoadFromFile
  JsonLoader-->>MainWindow: LocationPointList
  MainWindow->>Filter: ApplyFilter
  Filter-->>MainWindow: gefilterte Punkte
  MainWindow->>MapWidget: SetPoints
  MainWindow->>MapWidget: CenterOnPoints
  MapWidget->>MapWidget: paintEvent Tiles plus Overlay
  MapWidget->>TileDownloader: fehlende Kacheln
  TileDownloader-->>MapWidget: TileDownloaded
  User->>MapWidget: Klick auf Punkt
  MapWidget-->>MainWindow: PointClicked
```

1. Datei laden → flache Punktliste (`LocationPoint`).
2. Filter (Datum, Wochentag, Uhrzeit) erzeugen `_filteredPoints`.
3. `MapWidget` zentriert auf die dichteste Zelle (`ComputeDensestFocus`) und zeichnet OSM-Kacheln plus Overlay je `DisplayMode`.
4. Klick trifft den nächsten Punkt im Pixelradius und füllt die Punktinfo.

Details: [core.md](core.md), [map.md](map.md), [ui.md](ui.md).

## Verzeichnislayout

```text
src/           Kern + Qt-UI
tests/         gtest, Fixture sample_timeline.json
third_party/   googletest (Git-Submodul)
doc/           diese Dokumentation
```

Abhängigkeiten außerhalb des Repos: Qt 6. nlohmann/json kommt per FetchContent, gtest per Submodul. Siehe Root-[README.md](../README.md).
