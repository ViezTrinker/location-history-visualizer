/*!
 *\file geojson_exporter.cpp
 *\brief Writes filtered location points as GeoJSON
 */

#include "geojson_exporter.h"

#include <cstdint>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "civil_time.h"
#include "export_result.h"
#include "location_data.h"
#include "location_point.h"

namespace LocationHistory
{
   namespace
   {
      inline constexpr int32_t JsonIndentSpaces = 2;

      std::string_view SourceName(const PointSource source)
      {
         if (source == PointSource::TimelinePath)
         {
            return "timelinePath";
         }
         if (source == PointSource::Visit)
         {
            return "visit";
         }
         return "rawPosition";
      }
   } // namespace

   ExportResult WriteGeoJson(const LocationPointList& points, std::string& output)
   {
      output.clear();
      if (points.empty())
      {
         return ExportResult::NoPoints;
      }

      nlohmann::json collection;
      collection["type"] = "FeatureCollection";
      collection["features"] = nlohmann::json::array();

      for (size_t index = 0; index < points.size(); ++index)
      {
         const LocationPoint& point = points[index];
         std::string timeText;
         FormatIso8601(point.unixTimeMs, point.utcOffsetMinutes, timeText);

         nlohmann::json feature;
         feature["type"] = "Feature";
         feature["geometry"]["type"] = "Point";
         feature["geometry"]["coordinates"] = nlohmann::json::array({point.longitude, point.latitude});
         feature["properties"]["time"] = timeText;
         feature["properties"]["source"] = std::string(SourceName(point.source));
         if (PointHasDuration(point))
         {
            std::string endTimeText;
            FormatIso8601(point.endUnixTimeMs, point.utcOffsetMinutes, endTimeText);
            feature["properties"]["endTime"] = endTimeText;
         }
         if (point.pathId != NoPathId)
         {
            feature["properties"]["pathId"] = point.pathId;
         }
         collection["features"].push_back(feature);
      }

      output = collection.dump(JsonIndentSpaces);
      output.push_back('\n');
      return ExportResult::Ok;
   }
} // namespace LocationHistory
