/*!
 *\file gpx_exporter_test.cpp
 *\brief Unit tests for GPX export
 */

#include "gpx_exporter.h"

#include <string>

#include <gtest/gtest.h>

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

TEST(GpxExporter, EmptyListReturnsNoPoints)
{
   LocationHistory::LocationPointList points;
   std::string output;
   const LocationHistory::ExportResult result = LocationHistory::WriteGpx(points, output);
   EXPECT_EQ(result, LocationHistory::ExportResult::NoPoints);
   EXPECT_TRUE(output.empty());
}

TEST(GpxExporter, VisitBecomesWaypoint)
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
   const LocationHistory::ExportResult result = LocationHistory::WriteGpx(points, output);
   EXPECT_TRUE(LocationHistory::IsOk(result));
   EXPECT_NE(output.find("<wpt lat="), std::string::npos);
   EXPECT_NE(output.find("<name>Visit</name>"), std::string::npos);
   EXPECT_NE(output.find("<time>"), std::string::npos);
   EXPECT_NE(output.find("2020-01-15T08:30:00.000+01:00"), std::string::npos);
   EXPECT_EQ(output.find("<trk>"), std::string::npos);
}

TEST(GpxExporter, SamePathIdSharesOneTrack)
{
   LocationHistory::LocationPointList points;
   points.push_back(MakePoint(
      50.11,
      8.68,
      1579073400000,
      60,
      LocationHistory::PointSource::TimelinePath,
      1));
   points.push_back(MakePoint(
      50.12,
      8.69,
      1579073460000,
      60,
      LocationHistory::PointSource::TimelinePath,
      1));

   std::string output;
   ASSERT_TRUE(LocationHistory::IsOk(LocationHistory::WriteGpx(points, output)));
   EXPECT_NE(output.find("<trk>"), std::string::npos);
   EXPECT_NE(output.find("<name>Path 1</name>"), std::string::npos);

   size_t trackCount = 0;
   size_t searchIndex = 0;
   while (true)
   {
      const size_t found = output.find("<trk>", searchIndex);
      if (found == std::string::npos)
      {
         break;
      }
      trackCount += 1;
      searchIndex = found + 1;
   }
   EXPECT_EQ(trackCount, 1u);

   size_t pointCount = 0;
   searchIndex = 0;
   while (true)
   {
      const size_t found = output.find("<trkpt ", searchIndex);
      if (found == std::string::npos)
      {
         break;
      }
      pointCount += 1;
      searchIndex = found + 1;
   }
   EXPECT_EQ(pointCount, 2u);
}
