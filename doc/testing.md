# Tests

The tests link only `location_history_core` and `GTest::gtest_main`. No Qt, no network tiles.

GoogleTest lives as a submodule under `third_party/googletest`. If `CMakeLists.txt` is missing there, CMake stops with a hint to run `git submodule update --init --recursive`.

## Mapping

| Test file | Module | Examples |
| --- | --- | --- |
| `tests/tile_math_test.cpp` | `tile_math` | lat/lng round-trip, zoom clamp, tile bounds |
| `tests/json_loader_test.cpp` | `json_loader`, `civil_time` | LatLng string, ISO-8601, fixture, path ids, visit duration, error codes |
| `tests/location_filter_test.cpp` | `location_filter` | date, weekday, time-of-day window |
| `tests/clusterer_test.cpp` | `clusterer` | identical points, separated points, empty input |
| `tests/story_time_test.cpp` | `story_time` | slider ↔ timestamp, start day plus later days, visibility at cutoff |
| `tests/map_focus_test.cpp` | `map_focus` | empty list, densest cell, high zoom for a tight group |
| `tests/app_language_test.cpp` | `app_language` (header, no Qt) | language codes including ja/ko/id/vi/hi, English fallback, round-trip, text direction |
| `tests/app_theme_test.cpp` | `app_theme` (header, no Qt) | Dark default, light/midnight/nord/sepia codes, round-trip |
| `tests/map_display_settings_test.cpp` | `map_display_settings` (header, no Qt) | clamp, default 20000/4 px, downsampling step |

Fixture: [`tests/fixtures/sample_timeline.json`](../tests/fixtures/sample_timeline.json) — anonymized mini timeline with `timelinePath`, `visit`, and `rawSignals.position`. The path is supplied as the compile definition `TEST_FIXTURE_DIR`.

## Running

```text
cmake --build build --config Release --target location_history_visualizer_tests
.\build\Release\location_history_visualizer_tests.exe
```

Or `ctest -C Release` from the build folder (`gtest_discover_tests`, `DISCOVERY_MODE PRE_TEST`).
