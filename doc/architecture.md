# Architecture

The visualizer is a native Windows desktop app in C++20. The GUI sits on Qt 6. The map is OpenStreetMap raster tiles plus a self-drawn overlay.

All domain code lives in the `LocationHistory` namespace. `src/main.cpp` only starts `QApplication` and `MainWindow`.

## Layers

CMake splits the code into three targets. The core library does not use Qt. The tests link only the core.

```mermaid
flowchart TB
  subgraph app [location_history_visualizer]
    mainCpp[main.cpp]
    mainWindow[MainWindow]
    mapWidget[MapWidget]
    aboutDialog[AboutDialog]
    jsonLoadThread[JsonLoadThread]
    appLanguage[app_language]
    appTheme[app_theme]
    mapDisplaySettings[map_display_settings]
    tileCache[TileCache]
    tileDownloader[TileDownloader]
  end

  subgraph core [location_history_core]
    jsonLoader[json_loader]
    filter[location_filter]
    civilTime[civil_time]
    tileMath[tile_math]
    clusterer[clusterer]
    storyTime[story_time]
    mapFocus[map_focus]
    gpxExporter[gpx_exporter]
    geojsonExporter[geojson_exporter]
    dataModel[location_point / location_data]
  end

  subgraph tests [location_history_visualizer_tests]
    gtest[gtest]
  end

  mainCpp --> mainWindow
  mainCpp --> appLanguage
  mainCpp --> appTheme
  mainWindow --> mapWidget
  mainWindow --> aboutDialog
  mainWindow --> appLanguage
  mainWindow --> appTheme
  mainWindow --> mapDisplaySettings
  mapWidget --> mapDisplaySettings
  mainWindow --> jsonLoadThread
  jsonLoadThread --> jsonLoader
  mainWindow --> filter
  mainWindow --> storyTime
  mainWindow --> gpxExporter
  mainWindow --> geojsonExporter
  mapWidget --> tileCache
  mapWidget --> tileDownloader
  mapWidget --> tileMath
  mapWidget --> clusterer
  mapWidget --> storyTime
  mapWidget --> mapFocus
  jsonLoader --> dataModel
  jsonLoader --> civilTime
  gpxExporter --> civilTime
  geojsonExporter --> civilTime
  filter --> dataModel
  filter --> civilTime
  tests --> core
  tests --> gtest
```

| Target | Role | Qt |
| --- | --- | --- |
| `location_history_core` | Static library: parse, filter, projection, visualization math | no |
| `location_history_visualizer` | `.exe`: window, tiles, drawing | Widgets + Network |
| `location_history_visualizer_tests` | gtest against the core | no |

## Runtime flow

```mermaid
sequenceDiagram
  participant User
  participant MainWindow
  participant JsonLoadThread
  participant JsonLoader
  participant Filter
  participant MapWidget
  participant TileDownloader

  User->>MainWindow: Open JSON
  MainWindow->>JsonLoadThread: start
  JsonLoadThread->>JsonLoader: LoadFromFile
  JsonLoader-->>JsonLoadThread: progress / result
  JsonLoadThread-->>MainWindow: finished
  MainWindow->>Filter: ApplyFilter
  Filter-->>MainWindow: filtered points
  MainWindow->>MapWidget: SetPoints
  MainWindow->>MapWidget: CenterOnPoints
  MapWidget->>MapWidget: paintEvent tiles plus overlay
  MapWidget->>TileDownloader: missing tiles
  TileDownloader-->>MapWidget: TileDownloaded
  User->>MapWidget: click a point
  MapWidget-->>MainWindow: PointClicked
```

1. Load a file on a worker thread → flat point list (`LocationPoint`). The UI shows progress and can cancel.
2. Filters (date, weekday, time of day) produce `_filteredPoints`.
3. `MapWidget` centers on the densest cell (`ComputeDensestFocus`) and draws OSM tiles plus the overlay for the current `DisplayMode`.
4. A click hits the nearest point within the pixel radius and fills the point-info panel.

Details: [core.md](core.md), [map.md](map.md), [ui.md](ui.md).

## Directory layout

```text
src/           entry point (main.cpp)
src/core/      Qt-free library (parse, filter, export, overlay math)
src/ui/        Qt window, settings, load thread
src/map/       MapWidget and OSM tile cache/downloader
tests/         gtest, fixture sample_timeline.json
third_party/   googletest (Git submodule)
doc/           this documentation
```

Dependencies outside the repo: Qt 6. nlohmann/json comes via FetchContent, gtest via submodule. See the root [README.md](../README.md).
