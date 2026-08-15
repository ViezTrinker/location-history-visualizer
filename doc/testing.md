# Tests

Die Tests linken nur `location_history_core` und `GTest::gtest_main`. Kein Qt, keine Netzwerk-Kacheln.

GoogleTest liegt als Submodul unter `third_party/googletest`. Fehlt `CMakeLists.txt` dort, bricht CMake mit dem Hinweis auf `git submodule update --init --recursive` ab.

## Zuordnung

| Testdatei | Modul | Beispiele |
| --- | --- | --- |
| `tests/tile_math_test.cpp` | `tile_math` | Roundtrip Lat/Lng, Zoom-Clamp, Kachelgrenzen |
| `tests/json_loader_test.cpp` | `json_loader`, `civil_time` | LatLng-String, ISO-8601, Fixture, Pfad-IDs, Visit-Dauer, Fehlercodes |
| `tests/location_filter_test.cpp` | `location_filter` | Datum, Wochentag, Uhrzeitfenster |
| `tests/clusterer_test.cpp` | `clusterer` | identische Punkte, getrennte Punkte, leere Eingabe |
| `tests/story_time_test.cpp` | `story_time` | Slider ↔ Zeitstempel, Starttag plus folgende Tage, Sichtbarkeit am Cutoff |
| `tests/map_focus_test.cpp` | `map_focus` | leere Liste, dichteste Zelle, hoher Zoom bei enger Gruppe |
| `tests/app_language_test.cpp` | `app_language` (header, ohne Qt) | Sprachcodes, Fallback Englisch, Roundtrip |

Fixture: [`tests/fixtures/sample_timeline.json`](../tests/fixtures/sample_timeline.json) — anonymisierte Mini-Timeline mit `timelinePath`, `visit` und `rawSignals.position`. Der Pfad kommt als Compile-Definition `TEST_FIXTURE_DIR`.

## Ausführen

```text
cmake --build build --config Release --target location_history_visualizer_tests
.\build\Release\location_history_visualizer_tests.exe
```

Oder `ctest -C Release` im Build-Ordner (`gtest_discover_tests`, `DISCOVERY_MODE PRE_TEST`).
