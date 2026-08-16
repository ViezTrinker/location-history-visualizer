# Map and overlay

The map is not a Google Maps widget. `MapWidget` computes Web Mercator itself, fetches OSM PNGs, and draws locations on top.

## Projection

[`src/core/tile_math.h`](../src/core/tile_math.h) converts geographic coordinates to world pixels, matching OSM:

- Tile size 256 px
- Zoom 2..19
- Latitude clamped to ±85.05112878° (Mercator pole)

`MapWidget` stores the view as `_centerWorldX` / `_centerWorldY` plus `_zoom`. Mouse wheel and double-click zoom around the cursor (world coordinates scale by `2^(newZoom-oldZoom)`). The zoom bar in `MainWindow` (`+` / slider / `-`) zooms around the map center and stays in sync via `ZoomChanged`. Dragging shifts the center in pixels.

After load, `CenterOnPoints` focuses on the densest cell, not the bounding box. The calculation lives in [`src/core/map_focus.h`](../src/core/map_focus.h) (`ComputeDensestFocus`, grid ~0.02°).

## Tile pipeline

```mermaid
flowchart LR
  paint[paintEvent] --> visible[visible TileIds]
  visible --> mem[TileCache RAM LRU]
  mem --> disk[TileCache disk]
  disk --> http[TileDownloader]
  http --> osm["tile.openstreetmap.org/z/x/y.png"]
  mem --> blit[QPainter drawPixmap]
```

[`src/map/tile_cache.h`](../src/map/tile_cache.h): up to `MaxMemoryTiles` (256) in RAM, LRU eviction. Disk under `%LOCALAPPDATA%/<AppName>/tiles/{z}/{x}/{y}.png`.

[`src/map/tile_downloader.h`](../src/map/tile_downloader.h): `QNetworkAccessManager`, at most **2** parallel requests (OSM tile policy), fixed User-Agent. A queue plus an in-flight set prevent duplicate downloads. Finished PNGs go to `MapWidget` via the `TileDownloaded` signal, which writes the cache and calls `update()`.

Missing tiles appear gray until the download arrives. Bottom left: `© OpenStreetMap contributors`. File → Export → Map image captures the `MapWidget` as shown (tiles, overlay, Story frame, attribution), so missing tiles stay gray in the file and attribution remains in the image.

## Overlay modes

[`src/map/map_widget.h`](../src/map/map_widget.h) draws tiles first in `paintEvent`, then the current `DisplayMode`. The time cutoff (`SetUntilTime`) applies only in `Story`.

| Mode | Behavior |
| --- | --- |
| `Points` | simple red circles, viewport culling. Point size and drawn-point cap come from the sidebar (Map display; default 4 px and 20 000) |
| `Clustered` | `BuildClusters` at the current zoom, circle size ~ log(count). Click a circle to zoom onto that cell |
| `Story` | red points as in `Points` (same size and cap), but timed: first the start day, then all later days while Play is running |

## Hit testing

Left-click without a meaningful drag:

- **Points / Story:** nearest visible point within `HitTestRadiusPx` (12); visits are slightly larger. Signal `PointClicked(lat, lng, unixTimeMs, utcOffsetMinutes, endUnixTimeMs, source)` or `PointCleared`. The selected point gets a ring.
- **Clustered:** nearest cluster circle using the same radius as drawing. On a hit, `ComputeSpanFocus` recenters on the cluster and zooms to fit its bounding box (with the same padding as post-load focus). If that zoom is not deeper than the current level, zoom increases by one (clamped to `MaxZoom`). A miss leaves the view unchanged and does not change point selection. Any previous point selection is cleared (`PointCleared`).
