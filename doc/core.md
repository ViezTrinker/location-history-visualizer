# Kernbibliothek

`location_history_core` enthält die Logik ohne Qt. Die Unit-Tests decken genau diese Schicht ab.

## Datenmodell

[`src/location_point.h`](../src/location_point.h) ist die kleinste Einheit:

| Feld | Bedeutung |
| --- | --- |
| `latitude` / `longitude` | WGS84, Grad |
| `unixTimeMs` | Unix-Zeit in Millisekunden (UTC), Start des Samples |
| `utcOffsetMinutes` | Offset aus dem JSON-Zeitstempel (`+02:00` → 120) |
| `source` | `TimelinePath`, `Visit` oder `RawPosition` |
| `endUnixTimeMs` | Ende eines Aufenthalts; bei Pfadpunkten gleich `unixTimeMs` |
| `pathId` | gemeinsame ID aller Punkte eines `timelinePath`-Arrays, sonst `NoPathId` |

[`src/location_data.h`](../src/location_data.h) definiert `LocationPointList` und `DisplayMode` (`Points`, `Clustered`, `Story`).

Fehlercodes stehen in [`src/load_result.h`](../src/load_result.h): `LoadResult` / `ParseResult` plus `IsOk` / `IsErr` / `IsMsg`. Kein `bool` als Erfolg/Fehler nach außen.

## JSON-Import

[`src/json_loader.h`](../src/json_loader.h) liest den **aktuellen Google-Timeline-Export** (nicht das alte `locations[]` mit E7-Koordinaten).

Koordinaten kommen als Text: `"50.1109°, 8.6821°"`. Zeiten als ISO-8601 mit Offset, z.B. `2020-01-15T08:30:00.000+01:00`.

Der Parser ist ein nlohmann-JSON-**SAX**-Handler, damit große Dateien nicht als voller DOM im RAM liegen. Er sammelt:

| JSON-Pfad | `PointSource` |
| --- | --- |
| `semanticSegments[].timelinePath[].point` + `time` | `TimelinePath` (eine `pathId` pro Array) |
| `semanticSegments[].visit.topCandidate.placeLocation.latLng` + Segment-`startTime`/`endTime` | `Visit` |
| `rawSignals[].position.LatLng` + `timestamp` | `RawPosition` |

Ignoriert in v1: `activity`, `timelineMemory`, `userLocationProfile`.

`LoadFromFile` / `LoadFromString` liefern `FileNotFound`, `InvalidJson`, `NoPoints` oder `Ok`.

## Zeit und Filter

[`src/civil_time.h`](../src/civil_time.h) wandelt Civil-Datum ↔ Unix-ms, Wochentag (Montag = 0) und Minute des Tages. Wochentag und Uhrzeit nutzen den **Offset im Zeitstempel**, nicht die Zeitzone des PCs.

[`src/location_filter.h`](../src/location_filter.h): `FilterSettings` mit Datum von/bis, `weekdayMask` (Bitflags) und Minutenfenster. `ApplyFilter` schreibt die Treffer nach `output`. Über Mitternacht: `fromMinuteOfDay > toMinuteOfDay` gilt als wrap-around.

## Cluster

[`src/clusterer.h`](../src/clusterer.h) rastert Punkte in Weltpixeln am aktuellen Zoom. Zellengröße standardmäßig `ClusterCellSizePx` (48). Pro Zelle: Mittelwert der Koordinaten und `count`.

## Story-Zeit

[`src/story_time.h`](../src/story_time.h) mappt den Scrubber (0..`StorySliderMax`) vom gewählten Starttag bis zum letzten späteren Sample. `CollectPointsFromDate` hält alles ab diesem lokalen Datum. `LastTimeOnCivilDate` setzt die Startposition auf das Ende des Starttags. `PointVisibleUntil` zeigt ein Sample, sobald sein Start (`unixTimeMs`) den Cutoff erreicht hat.

## Kartenfokus

[`src/map_focus.h`](../src/map_focus.h) wählt nach dem Laden die Zelle mit den meisten Punkten (`DensityCellDegrees` 0.02, grob 2 km). `ComputeDensestFocus` liefert Mittelpunkt und Zoom, der die Zelle plus Padding ins Viewport legt. `FocusResult`: `NoPoints` oder `Ok`.

## Version

[`src/version.h`](../src/version.h) hält About-Strings (`AppVersion` `1.1.0.R`, `ReleaseDate` `2026-08-15`, Author- und Repo-URL `https://github.com/ViezTrinker/location-history-visualizer`). Keine Qt-Abhängigkeit.
