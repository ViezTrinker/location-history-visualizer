/*!
 *\file version.h
 *\brief Application version and About-dialog strings
 */

#ifndef VERSION_H
#define VERSION_H

#include <string_view>

namespace LocationHistory
{
   inline constexpr std::string_view AppName = "Location History Visualizer";
   inline constexpr std::string_view AppVersion = "00.01.00.A";
   inline constexpr std::string_view ReleaseDate = "2026-08-14";
   inline constexpr std::string_view AuthorName = "ViezTrinker";
   inline constexpr std::string_view AuthorUrl = "https://github.com/ViezTrinker";
   inline constexpr std::string_view RepositoryUrl = "https://github.com/ViezTrinker/location_history_visualizer";
} // namespace LocationHistory

#endif // VERSION_H
