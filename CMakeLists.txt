cmake_minimum_required(VERSION 3.20)

project(location_history_visualizer VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC_SOURCE_GROUP "Generated")
set(CMAKE_AUTORCC_SOURCE_GROUP "Generated")
set(CMAKE_AUTOGEN_SOURCE_GROUP "Generated")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

if(MSVC)
   add_compile_options(/utf-8)
endif()

if(WIN32)
   file(GLOB Qt6MsvcRoots "C:/Qt/6.*/msvc*_64")
   foreach(qtRoot IN LISTS Qt6MsvcRoots)
      if(EXISTS "${qtRoot}/lib/cmake/Qt6/Qt6Config.cmake")
         list(APPEND CMAKE_PREFIX_PATH "${qtRoot}")
      endif()
   endforeach()
endif()

find_package(Qt6 QUIET COMPONENTS Widgets Network LinguistTools)
if(NOT Qt6_FOUND)
   message(FATAL_ERROR
      "Qt 6 (Widgets + Network + LinguistTools) was not found.\n"
      "Qt is not shipped with this repository; install it locally, then point CMake at the kit.\n"
      "\n"
      "Windows installer: https://www.qt.io/download-qt-installer\n"
      "  Choose Qt 6.x and an MSVC 64-bit kit (for example msvc2022_64).\n"
      "\n"
      "Windows command line:\n"
      "  python -m pip install aqtinstall\n"
      "  python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir C:/Qt\n"
      "\n"
      "Reconfigure with the kit path:\n"
      "  cmake -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64\n"
      "Or set the CMAKE_PREFIX_PATH environment variable to that directory.\n")
endif()

include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_Declare(
   nlohmann_json
   GIT_REPOSITORY https://github.com/nlohmann/json.git
   GIT_TAG v3.11.3
   GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

set(LOCATION_HISTORY_CORE_SOURCES
   src/core/civil_time.cpp
   src/core/civil_time.h
   src/core/clusterer.cpp
   src/core/clusterer.h
   src/core/export_result.h
   src/core/geojson_exporter.cpp
   src/core/geojson_exporter.h
   src/core/gpx_exporter.cpp
   src/core/gpx_exporter.h
   src/core/json_loader.cpp
   src/core/json_loader.h
   src/core/load_observer.h
   src/core/load_result.h
   src/core/location_data.h
   src/core/location_filter.cpp
   src/core/location_filter.h
   src/core/location_point.h
   src/core/map_focus.cpp
   src/core/map_focus.h
   src/core/story_time.cpp
   src/core/story_time.h
   src/core/tile_math.cpp
   src/core/tile_math.h
   src/core/version.h
)

add_library(location_history_core STATIC
   ${LOCATION_HISTORY_CORE_SOURCES}
)
target_include_directories(location_history_core PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)
target_link_libraries(location_history_core PUBLIC
   nlohmann_json::nlohmann_json
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${LOCATION_HISTORY_CORE_SOURCES})

set(LOCATION_HISTORY_APP_SOURCES
   resources/app.qrc
   src/main.cpp
   src/map/map_widget.cpp
   src/map/map_widget.h
   src/map/tile_cache.cpp
   src/map/tile_cache.h
   src/map/tile_downloader.cpp
   src/map/tile_downloader.h
   src/ui/about_dialog.cpp
   src/ui/about_dialog.h
   src/ui/app_language.cpp
   src/ui/app_language.h
   src/ui/app_theme.cpp
   src/ui/app_theme.h
   src/ui/json_load_thread.cpp
   src/ui/json_load_thread.h
   src/ui/main_window.cpp
   src/ui/main_window.h
   src/ui/map_display_settings.cpp
   src/ui/map_display_settings.h
)
if(WIN32)
   list(APPEND LOCATION_HISTORY_APP_SOURCES
      resources/app.rc
   )
   set_source_files_properties(resources/app.rc PROPERTIES
      OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.ico"
   )
endif()

add_executable(location_history_visualizer WIN32
   ${LOCATION_HISTORY_APP_SOURCES}
)
target_include_directories(location_history_visualizer PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/src/map
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${LOCATION_HISTORY_APP_SOURCES})
target_link_libraries(location_history_visualizer PRIVATE
   location_history_core
   Qt6::Widgets
   Qt6::Network
)

qt_add_translations(location_history_visualizer
   SOURCE_TARGETS location_history_visualizer
   TS_FILES
      translations/location_history_visualizer_ar.ts
      translations/location_history_visualizer_de.ts
      translations/location_history_visualizer_es.ts
      translations/location_history_visualizer_fr.ts
      translations/location_history_visualizer_hi.ts
      translations/location_history_visualizer_id.ts
      translations/location_history_visualizer_it.ts
      translations/location_history_visualizer_ja.ts
      translations/location_history_visualizer_ko.ts
      translations/location_history_visualizer_nl.ts
      translations/location_history_visualizer_pl.ts
      translations/location_history_visualizer_pt.ts
      translations/location_history_visualizer_ru.ts
      translations/location_history_visualizer_tr.ts
      translations/location_history_visualizer_vi.ts
   NO_GENERATE_PLURALS_TS_FILE
   RESOURCE_PREFIX "/i18n"
)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest/CMakeLists.txt")
   message(FATAL_ERROR
      "The googletest submodule is missing.\n"
      "Clone with: git clone --recurse-submodules <url>\n"
      "Or in an existing clone: git submodule update --init --recursive\n")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/googletest)

enable_testing()

set(LOCATION_HISTORY_TEST_SOURCES
   tests/app_language_test.cpp
   tests/app_theme_test.cpp
   tests/clusterer_test.cpp
   tests/geojson_exporter_test.cpp
   tests/gpx_exporter_test.cpp
   tests/json_loader_test.cpp
   tests/location_filter_test.cpp
   tests/map_display_settings_test.cpp
   tests/map_focus_test.cpp
   tests/story_time_test.cpp
   tests/tile_math_test.cpp
)

add_executable(location_history_visualizer_tests
   ${LOCATION_HISTORY_TEST_SOURCES}
)
target_include_directories(location_history_visualizer_tests PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/src/core
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)
target_link_libraries(location_history_visualizer_tests PRIVATE
   location_history_core
   GTest::gtest_main
)
target_compile_definitions(location_history_visualizer_tests PRIVATE
   TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests" PREFIX "tests" FILES ${LOCATION_HISTORY_TEST_SOURCES})

set_target_properties(location_history_core PROPERTIES FOLDER "App")
set_target_properties(location_history_visualizer PROPERTIES FOLDER "App")
set_target_properties(location_history_visualizer_tests PROPERTIES FOLDER "Tests")
set_target_properties(gtest gtest_main PROPERTIES FOLDER "Third Party")

include(GoogleTest)
gtest_discover_tests(location_history_visualizer_tests DISCOVERY_MODE PRE_TEST)

if(WIN32)
   get_target_property(QT6_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
   get_filename_component(QT6_BIN_DIR "${QT6_QMAKE_EXECUTABLE}" DIRECTORY)
   find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT6_BIN_DIR}")
   if(WINDEPLOYQT_EXECUTABLE)
      add_custom_command(TARGET location_history_visualizer POST_BUILD
         COMMAND "${WINDEPLOYQT_EXECUTABLE}" --translations ar,de,es,fr,hi,id,it,ja,ko,nl,pl,pt,ru,tr,vi "$<TARGET_FILE:location_history_visualizer>"
         COMMENT "Deploying Qt runtime next to the executable"
      )
   endif()
endif()
