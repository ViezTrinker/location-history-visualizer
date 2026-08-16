"""
Build a GitHub Release zip from a Windows Qt build folder.

The zip contains the app exe plus the Qt runtime that windeployqt places
next to it. Tests, static libs, and debug symbols are left out.

Run from the repository root after a Release build:

    python scripts/make_release_zip.py
"""

from __future__ import annotations

import argparse
import re
import sys
import zipfile
from pathlib import Path

EXECUTABLE_NAME = "location_history_visualizer.exe"
PLATFORM_PLUGIN_NAME = "qwindows.dll"
SKIP_SUFFIXES = {".exp", ".ilk", ".lib", ".obj", ".pdb"}
SKIP_DIRECTORY_NAMES = {"cmakeFiles"}
VERSION_PATTERN = re.compile(
    r'inline constexpr std::string_view AppVersion = "([^"]+)"'
)


def repository_root() -> Path:
    return Path(__file__).resolve().parent.parent


def read_app_version(version_header: Path) -> str:
    text = version_header.read_text(encoding="utf-8")
    match = VERSION_PATTERN.search(text)
    if match is None:
        raise RuntimeError(f"AppVersion not found in {version_header}")
    return match.group(1)


def should_skip_directory(directory_name: str) -> bool:
    lowered = directory_name.lower()
    if lowered in SKIP_DIRECTORY_NAMES:
        return True
    if lowered.endswith(".dir"):
        return True
    return False


def should_skip_file(file_path: Path) -> bool:
    suffix = file_path.suffix.lower()
    if suffix in SKIP_SUFFIXES:
        return True
    name = file_path.name.lower()
    if name.endswith("_tests.exe"):
        return True
    return False


def collect_release_files(binary_dir: Path) -> list[Path]:
    selected: list[Path] = []
    for child in sorted(binary_dir.iterdir()):
        if child.is_dir():
            if should_skip_directory(child.name):
                continue
            for nested in sorted(child.rglob("*")):
                if nested.is_file() and not should_skip_file(nested):
                    selected.append(nested)
            continue
        if child.is_file() and not should_skip_file(child):
            selected.append(child)
    return selected


def zip_arcname(package_folder: str, binary_dir: Path, file_path: Path) -> str:
    relative = file_path.relative_to(binary_dir)
    return str(Path(package_folder) / relative).replace("\\", "/")


def write_zip(zip_path: Path, package_folder: str, binary_dir: Path, files: list[Path]) -> None:
    zip_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for file_path in files:
            archive.write(file_path, arcname=zip_arcname(package_folder, binary_dir, file_path))


def parse_arguments(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Pack location_history_visualizer.exe and the Qt runtime into a Release zip."
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=None,
        help="CMake build directory (default: <repo>/build)",
    )
    parser.add_argument(
        "--config",
        default="Release",
        help="CMake configuration folder under the build directory (default: Release)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Zip path (default: <repo>/dist/<name>-<version>-windows-x64.zip)",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    arguments = parse_arguments(argv)
    repo_root = repository_root()
    build_dir = arguments.build_dir
    if build_dir is None:
        build_dir = repo_root / "build"
    binary_dir = (build_dir / arguments.config).resolve()
    executable_path = binary_dir / EXECUTABLE_NAME
    if not executable_path.is_file():
        print(f"Executable not found: {executable_path}", file=sys.stderr)
        print("Build the Release target first so windeployqt can copy the Qt runtime.", file=sys.stderr)
        return 1

    platform_plugin = binary_dir / "platforms" / PLATFORM_PLUGIN_NAME
    if not platform_plugin.is_file():
        print(f"Missing {platform_plugin}", file=sys.stderr)
        print("The Qt platform plugin is required. Rebuild so the windeployqt post-build step runs.", file=sys.stderr)
        return 1

    version = read_app_version(repo_root / "src" / "core" / "version.h")
    package_folder = f"location-history-visualizer-{version}-windows-x64"
    output_path = arguments.output
    if output_path is None:
        output_path = repo_root / "dist" / f"{package_folder}.zip"
    output_path = output_path.resolve()

    files = collect_release_files(binary_dir)
    if executable_path not in files:
        files.insert(0, executable_path)

    write_zip(output_path, package_folder, binary_dir, files)
    print(f"Wrote {output_path}")
    print(f"Packed {len(files)} files. Upload this zip to the GitHub Release.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
