# Changelog

All notable releases of Location History Visualizer are listed here.

## Unreleased

## 2.0.0.R — 2026-08-16

Export of filtered points and the map view. Background JSON load, cluster-click zoom, and remembered window geometry from the 1.1 line.

### Added

- File → Export: GPX, GeoJSON, and a screenshot of the map (PNG, JPEG, BMP, WebP when the Qt plugin is present).
- In Cluster mode, clicking a circle zooms onto that cell (same fit as the post-load focus). If the view is already as tight as the cell, zoom increases by one level.
- Timeline JSON loads on a background thread with a progress dialog and Cancel. A failed or cancelled load leaves the previously shown data unchanged.

### Changed

- Window size, position, and maximized state are restored on the next start instead of always opening maximized.
- CMake `project(... VERSION ...)` is `2.0.0`, matching `AppVersion` `2.0.0.R` (CMake versions are numeric only).
- Source files are grouped under `src/core`, `src/ui`, and `src/map`. Visual Studio Solution Explorer follows those folders.

## 1.1.0.R — 2026-08-15

Display modes, sidebar layout, themes, and extra UI languages.

### Changed

- Points mode draws filtered samples as simple red dots. Cluster is unchanged.
- Story starts on a chosen day. Play reveals later days as raw red points. Heatmap and Blur are removed.
- Language and Theme live under Settings in the menu bar.
- Point size and the drawn-point cap (default 20 000) are adjustable in the sidebar under Map display.
- Weekday filters show the abbreviation under each checkbox so two-letter labels are not clipped.
- Opening a JSON file is only in File → Open. The sidebar no longer has an Open button or the loaded file path.
- The window starts maximized, with the normal window frame.

### Added

- Russian, Arabic, Italian, Turkish, Dutch, Portuguese, Polish, Japanese, Korean, Indonesian, Vietnamese, and Hindi UI translations. Arabic mirrors the layout right-to-left.
- Light, Midnight, Nord, and Sepia themes. Dark remains the default. The choice is remembered.
- The sidebar shows how many points the JSON contains and how many are currently visible.

## 1.0.0.R — 2026-08-14

First public release: a native Windows desktop app that plots a Google Timeline JSON export on OpenStreetMap tiles. Parsing and visualization run locally; the map only downloads OSM raster tiles.

### Added

- Load the current Google Timeline JSON (`semanticSegments` timeline paths and visits, `rawSignals.position`). The old Takeout `locations[]` E7 format is not supported.
- Interactive OpenStreetMap map: pan, mouse-wheel zoom around the cursor, double-click to zoom in, zoom slider beside the map.
- OSM tile pipeline with RAM + disk cache, at most two parallel downloads, a identifying User-Agent, and map attribution.
- Filters for date range, weekday, and time of day. The date range is filled from the loaded file.
- Display modes: all points, clusters, heatmap, and blur. A logarithmic scaling slider lifts rarer places in heatmap and blur so frequent locations do not saturate the view.
- Click a point to show local time, latitude, and longitude.
- After load, the map centers on the densest cluster instead of the bounding-box center.
- UI languages: English (default), German, Spanish, French. The choice is remembered.
- Last opened JSON path is remembered for the file dialog.
- Help → About with version, date, author, and repository link.

### Build and tests

- C++20, Qt 6 Widgets/Network, CMake, Visual Studio (x64). nlohmann/json via FetchContent, GoogleTest as a git submodule.
- Unit tests cover the Qt-free core (JSON loader, filters, clustering, heatmap math, map focus, language codes).

### Notes

- Documented target is Windows with an MSVC Qt 6 kit. Build from source; this tag does not ship a prebuilt installer.
- Location JSON never leaves the machine. Tile requests reveal the current map viewport to OSM. Follow the [OSM tile usage policy](https://operations.osmfoundation.org/policies/tiles/).
