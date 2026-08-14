# Location History Visualizer

C++ / Qt 6 desktop app that plots Google Timeline JSON on OpenStreetMap tiles.

Architecture: [doc/README.md](doc/README.md).

## Dependencies

These are **not** all in the Git repo:

| Dependency | How it is obtained |
| --- | --- |
| **Qt 6** (Widgets + Network) | You install it. Too large and compiler-specific to vendor. |
| **nlohmann/json** | CMake FetchContent (needs network on first configure). |
| **GoogleTest** | Git submodule `third_party/googletest`. |

## Clone

```text
git clone --recurse-submodules <repository-url>
```

If you already cloned without submodules:

```text
git submodule update --init --recursive
```

## Install Qt 6

Official installer: <https://www.qt.io/download-qt-installer>

Select a **Qt 6.x MSVC 64-bit** kit, for example `msvc2022_64`.

Or from a terminal:

```text
python -m pip install aqtinstall
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir C:/Qt
```

If CMake cannot find Qt, pass the kit directory:

```text
cmake -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build --config Release
```

On this machine, CMake also looks under `C:/Qt/6.*/msvc*_64` automatically.

For a machine-specific Visual Studio preset, copy `CMakeUserPresets.json.example` to `CMakeUserPresets.json` and edit the Qt path. That file is gitignored.

In Visual Studio (Open Folder) the top dropdown lists CMake **presets**, not the classic Debug/Release box. Choose **Release** (`vs-x64-release`) there, then build. If the list is stale: Project → Delete Cache and Reconfigure.

## Tests

```text
.\build\Release\location_history_visualizer_tests.exe
```

## License

This project is licensed under the [MIT License](LICENSE).

Third-party components keep their own licenses (Qt, nlohmann/json, GoogleTest, OpenStreetMap tiles).
