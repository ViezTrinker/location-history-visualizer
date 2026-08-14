# Kernbibliothek

`location_history_core` enthält die Logik ohne Qt. Die Unit-Tests decken genau diese Schicht ab.

## Datenmodell

[`src/location_point.h`](../src/location_point.h) ist die kleinste Einheit:

| Feld | Bedeutung |
| --- | --- |
| `latitude` / `longitude` | WGS84, Grad |
| `unixTimeMs` | Unix-Zeit in Millisekunden (UTC) |
| `utcOffsetMinutes` | Offset aus dem JSON-Zeitstempel (`+02:00` → 120) |
| `source` | `TimelinePath`, `Visit` oder `RawPosition` |

[`src/location_data.h`](../src/location_data.h) definiert `LocationPointList` und `DisplayMode` (`AllPoints`, `Clustered`, `Heatmap`, `Blur`).

Fehlercodes stehen in [`src/load_result.h`](../src/load_result.h): `LoadResult` / `ParseResult` plus `IsOk` / `IsErr` / `IsMsg`. Kein `bool` als Erfolg/Fehler nach außen.

## JSON-Import

[`src/json_loader.h`](../src/json_loader.h) liest den **aktuellen Google-Timeline-Export** (nicht das alte `locations[]` mit E7-Koordinaten).

Koordinaten kommen als Text: `"50.1109°, 8.6821°"`. Zeiten als ISO-8601 mit Offset, z.B. `2020-01-15T08:30:00.000+01:00`.

Der Parser ist ein nlohmann-JSON-**SAX**-Handler, damit große Dateien nicht als voller DOM im RAM liegen. Er sammelt:

| JSON-Pfad | `PointSource` |
| --- | --- |
| `semanticSegments[].timelinePath[].point` + `time` | `TimelinePath` |
| `semanticSegments[].visit.topCandidate.placeLocation.latLng` + Segment-`startTime` | `Visit` |
| `rawSignals[].position.LatLng` + `timestamp` | `RawPosition` |

Ignoriert in v1: `activity`, `timelineMemory`, `userLocationProfile`.

`LoadFromFile` / `LoadFromString` liefern `FileNotFound`, `InvalidJson`, `NoPoints` oder `Ok`.

## Zeit und Filter

[`src/civil_time.h`](../src/civil_time.h) wandelt Civil-Datum ↔ Unix-ms, Wochentag (Montag = 0) und Minute des Tages. Wochentag und Uhrzeit nutzen den **Offset im Zeitstempel**, nicht die Zeitzone des PCs.

[`src/location_filter.h`](../src/location_filter.h): `FilterSettings` mit Datum von/bis, `weekdayMask` (Bitflags) und Minutenfenster. `ApplyFilter` schreibt die Treffer nach `output`. Über Mitternacht: `fromMinuteOfDay > toMinuteOfDay` gilt als wrap-around.

## Cluster

[`src/clusterer.h`](../src/clusterer.h) rastert Punkte in Weltpixeln am aktuellen Zoom. Zellengröße standardmäßig `ClusterCellSizePx` (48). Pro Zelle: Mittelwert der Koordinaten und `count`.

## Heatmap-Mathe

[`src/heatmap_renderer.h`](../src/heatmap_renderer.h) bleibt pixelbasiert und Qt-frei:

- `AddGaussianSpot` akkumuliert Intensität
- `ColorFromHeat` mappt 0..1 auf eine transparent→blau→gelb→rot-Rampe
- `GaussianBlur` ist separabel (horizontal, dann vertikal)
- `ScaledHeatCeiling(maxHeat, heatScale)` senkt die Normierungsdecke, damit seltene Orte sichtbar bleiben (`heatScale` 1..100, logarithmisch vom UI-Slider)

Die eigentliche `QImage`-Ausgabe macht `MapWidget`. Siehe [map.md](map.md).

## Version

[`src/version.h`](../src/version.h) hält About-Strings (`AppVersion` `00.01.00.A`, `ReleaseDate`, Author- und Repo-URL). Keine Qt-Abhängigkeit.
