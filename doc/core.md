# Core library

`location_history_core` holds the logic without Qt. The unit tests cover this layer only.

## Data model

[`src/core/location_point.h`](../src/core/location_point.h) is the smallest unit:

| Field | Meaning |
| --- | --- |
| `latitude` / `longitude` | WGS84, degrees |
| `unixTimeMs` | Unix time in milliseconds (UTC), start of the sample |
| `utcOffsetMinutes` | Offset from the JSON timestamp (`+02:00` → 120) |
| `source` | `TimelinePath`, `Visit`, or `RawPosition` |
| `endUnixTimeMs` | End of a stay; equal to `unixTimeMs` for path points |
| `pathId` | Shared id of all points in one `timelinePath` array, otherwise `NoPathId` |

[`src/core/location_data.h`](../src/core/location_data.h) defines `LocationPointList` and `DisplayMode` (`Points`, `Clustered`, `Story`).

Error codes live in [`src/core/load_result.h`](../src/core/load_result.h): `LoadResult` / `ParseResult` plus `IsOk` / `IsErr` / `IsMsg`. Success and failure are not returned as `bool`. `Cancelled` is an informational result (`IsMsg`), not an error.

## JSON import

[`src/core/json_loader.h`](../src/core/json_loader.h) reads the **current Google Timeline export** (not the old `locations[]` format with E7 coordinates).

Coordinates arrive as text: `"50.1109°, 8.6821°"`. Times are ISO-8601 with an offset, for example `2020-01-15T08:30:00.000+01:00`.

The parser is a nlohmann JSON **SAX** handler so large files are not held as a full DOM in RAM. It collects:

| JSON path | `PointSource` |
| --- | --- |
| `semanticSegments[].timelinePath[].point` + `time` | `TimelinePath` (one `pathId` per array) |
| `semanticSegments[].visit.topCandidate.placeLocation.latLng` + segment `startTime`/`endTime` | `Visit` |
| `rawSignals[].position.LatLng` + `timestamp` | `RawPosition` |

Ignored in v1: `activity`, `timelineMemory`, `userLocationProfile`.

`LoadFromFile` / `LoadFromString` return `FileNotFound`, `InvalidJson`, `NoPoints`, `Ok`, or `Cancelled`. An optional `LoadObserver` reports byte progress and can stop the parse. The SAX handler returns false when `IsCancelled` is true so a stop is not reported as invalid JSON.

## Time and filters

[`src/core/civil_time.h`](../src/core/civil_time.h) converts civil date ↔ Unix ms, weekday (Monday = 0), and minute of day. Weekday and time of day use the **offset in the timestamp**, not the PC time zone. `FormatIso8601` writes `2020-01-15T08:30:00.000+01:00` for GPX and GeoJSON.

[`src/core/location_filter.h`](../src/core/location_filter.h): `FilterSettings` with from/to dates, `weekdayMask` (bit flags), and a minute window. `ApplyFilter` writes matches to `output`. Across midnight: `fromMinuteOfDay > toMinuteOfDay` is treated as wrap-around.

## Export

[`src/core/gpx_exporter.h`](../src/core/gpx_exporter.h) writes GPX 1.1 from a point list: visits as waypoints, each `pathId` as a track, raw positions as one time-sorted track. [`src/core/geojson_exporter.h`](../src/core/geojson_exporter.h) writes a `FeatureCollection` of Point features (`[longitude, latitude]`), with `time`, `source`, optional `endTime` and `pathId`. Both use `ExportResult` in [`src/core/export_result.h`](../src/core/export_result.h): `WriteFailed`, `NoPoints`, `Ok`.

## Clusters

[`src/core/clusterer.h`](../src/core/clusterer.h) bins points in world pixels at the current zoom. Default cell size is `ClusterCellSizePx` (48). Per cell: mean coordinates, bounding box (`min`/`max` latitude and longitude), and `count`.

## Story time

[`src/core/story_time.h`](../src/core/story_time.h) maps the scrubber (0..`StorySliderMax`) from the chosen start day to the last later sample. `CollectPointsFromDate` keeps everything from that local date onward. `LastTimeOnCivilDate` sets the start position to the end of the start day. `PointVisibleUntil` shows a sample once its start (`unixTimeMs`) has reached the cutoff.

## Map focus

[`src/core/map_focus.h`](../src/core/map_focus.h) picks the cell with the most points after load (`DensityCellDegrees` 0.02, roughly 2 km). `ComputeDensestFocus` returns a center and a zoom that fits the cell plus padding into the viewport. `ComputeSpanFocus` does the same for an explicit center and bounding box (used when clicking a cluster). `FocusResult`: `NoPoints` or `Ok`.

## Version

[`src/core/version.h`](../src/core/version.h) holds About strings (`AppVersion` `2.0.0.R`, `ReleaseDate` `2026-08-16`, author and repo URL `https://github.com/ViezTrinker/location-history-visualizer`). No Qt dependency. CMake `project(... VERSION ...)` is `2.0.0` (numeric prefix of `AppVersion`; CMake does not allow the `.R` suffix).
