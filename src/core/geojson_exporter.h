/*!
 *\file geojson_exporter.h
 *\brief Writes filtered location points as GeoJSON
 */

#ifndef GEOJSON_EXPORTER_H
#define GEOJSON_EXPORTER_H

#include <string>

#include "export_result.h"
#include "location_data.h"

namespace LocationHistory
{
   /*!
    *\brief Serializes location points to a GeoJSON FeatureCollection
    *
    *\param[in] points Filtered location points
    *\param[out] output GeoJSON text
    */
   ExportResult WriteGeoJson(const LocationPointList& points, std::string& output);
} // namespace LocationHistory

#endif // GEOJSON_EXPORTER_H
