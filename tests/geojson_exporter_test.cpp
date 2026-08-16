/*!
 *\file geojson_exporter_test.cpp
 *\brief Unit tests for GeoJSON export
 */

#include "geojson_exporter.h"

#include <string>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "export_result.h"
#include "location_data.h"
#include "location_point.h"

namespace
{
   LocationHistory::LocationPoint MakePoint(
      const double latitude,
      const double longitude,
      const int64_t unixTimeMs,
      const int32_t utcOffsetMinutes,
      const LocationHistory::PointSource source,
      const int32_t pathId)
   {
      LocationHistory::LocationPoint point{};
      point.latitude = latitude;
      point.longitude = longitude;
      point.unixTimeMs = unixTimeMs;
      point.utcOffsetMinutes = utcOffsetMinutes;
      point.source = source;
      point.endUnixTimeMs = unixTimeMs;
      point.pathId = pathId;
      return point;
   }
} // namespace

TEST(GeoJsonExporter, EmptyListReturnsNoPoints)
{
   LocationHistory::LocationPointList points;
   std::string output;
   const LocationHistory::ExportResult result = LocationHistory::WriteGeoJson(points, output);
   EXPECT_EQ(result, LocationHistory::ExportResult::NoPoints);
   EXPECT_TRUE(output.empty());
}

TEST(GeoJsonExporter, WritesFeatureCollectionWithLonLatOrder)
{
   LocationHistory::LocationPointList points;
   points.push_back(MakePoint(
      50.11,
      8.68,
      1579073400000,
      60,
      LocationHistory::PointSource::TimelinePath,
      3));

   std::string output;
   ASSERT_TRUE(LocationHistory::IsOk(LocationHistory::WriteGeoJson(points, output)));

   const nlohmann::json document = nlohmann::json::parse(output);
   EXPECT_EQ(document["type"], "FeatureCollection");
   ASSERT_EQ(document["features"].size(), 1u);
   const nlohmann::json& feature = document["features"][0];
   EXPECT_EQ(feature["geometry"]["type"], "Point");
   ASSERT_EQ(feature["geometry"]["coordinates"].size(), 2u);
   EXPECT_DOUBLE_EQ(feature["geometry"]["coordinates"][0].get<double>(), 8.68);
   EXPECT_DOUBLE_EQ(feature["geometry"]["coordinates"][1].get<double>(), 50.11);
   EXPECT_EQ(feature["properties"]["source"], "timelinePath");
   EXPECT_EQ(feature["properties"]["pathId"], 3);
   EXPECT_EQ(feature["properties"]["time"], "2020-01-15T08:30:00.000+01:00");
   EXPECT_FALSE(feature["properties"].contains("endTime"));
}

TEST(GeoJsonExporter, VisitIncludesEndTime)
{
   LocationHistory::LocationPointList points;
   LocationHistory::LocationPoint visit = MakePoint(
      50.11,
      8.68,
      1579073400000,
      60,
      LocationHistory::PointSource::Visit,
      LocationHistory::NoPathId);
   visit.endUnixTimeMs = visit.unixTimeMs + 3600000;
   points.push_back(visit);

   std::string output;
   ASSERT_TRUE(LocationHistory::IsOk(LocationHistory::WriteGeoJson(points, output)));

   const nlohmann::json document = nlohmann::json::parse(output);
   const nlohmann::json& feature = document["features"][0];
   EXPECT_EQ(feature["properties"]["source"], "visit");
   EXPECT_EQ(feature["properties"]["endTime"], "2020-01-15T09:30:00.000+01:00");
   EXPECT_FALSE(feature["properties"].contains("pathId"));
}
