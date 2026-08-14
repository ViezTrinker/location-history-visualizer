/*!
 *\file json_loader.h
 *\brief SAX loader for Google Timeline JSON exports
 */

#ifndef JSON_LOADER_H
#define JSON_LOADER_H

#include <string_view>

#include "load_result.h"
#include "location_data.h"

namespace LocationHistory
{
   /*!
    *\brief Parses a latitude/longitude string of the form "51.29°, 9.45°"
    *
    *\param[in] text Coordinate text from Google Timeline JSON
    *\param[out] latitude Parsed latitude in degrees
    *\param[out] longitude Parsed longitude in degrees
    */
   ParseResult ParseLatLng(std::string_view text, double& latitude, double& longitude);

   /*!
    *\brief Loads location points from a Timeline JSON file
    *
    *\param[in] path Filesystem path to the JSON file
    *\param[out] points Parsed location points
    */
   LoadResult LoadFromFile(std::string_view path, LocationPointList& points);

   /*!
    *\brief Loads location points from a Timeline JSON string
    *
    *\param[in] jsonText JSON document text
    *\param[out] points Parsed location points
    */
   LoadResult LoadFromString(std::string_view jsonText, LocationPointList& points);
} // namespace LocationHistory

#endif // JSON_LOADER_H
