/*!
 *\file map_focus_test.cpp
 *\brief Unit tests for densest-location map focus
 */

#include "map_focus.h"

#include <gtest/gtest.h>

#include "location_data.h"
#include "location_point.h"
#include "tile_math.h"

namespace
{
   LocationHistory::LocationPoint MakePoint(const double latitude, const double longitude)
   {
      LocationHistory::LocationPoint point{};
      point.latitude = latitude;
      point.longitude = longitude;
      point.unixTimeMs = 0;
      point.utcOffsetMinutes = 0;
      point.source = LocationHistory::PointSource::TimelinePath;
      return point;
   }
} // namespace

TEST(MapFocus, EmptyListReturnsNoPoints)
{
   LocationHistory::LocationPointList points;
   LocationHistory::MapFocus focus{};
   const LocationHistory::FocusResult result =
      LocationHistory::ComputeDensestFocus(points, 800, 600, focus);
   EXPECT_TRUE(LocationHistory::IsErr(result));
   EXPECT_EQ(result, LocationHistory::FocusResult::NoPoints);
}

TEST(MapFocus, ChoosesCellWithMostPoints)
{
   LocationHistory::LocationPointList points;
   for (int32_t index = 0; index < 20; ++index)
   {
      points.push_back(MakePoint(50.11, 8.68));
   }
   points.push_back(MakePoint(52.52, 13.40));
   points.push_back(MakePoint(48.14, 11.58));

   LocationHistory::MapFocus focus{};
   const LocationHistory::FocusResult result =
      LocationHistory::ComputeDensestFocus(points, 800, 600, focus);
   EXPECT_TRUE(LocationHistory::IsOk(result));
   EXPECT_NEAR(focus.latitude, 50.11, 0.01);
   EXPECT_NEAR(focus.longitude, 8.68, 0.01);
   EXPECT_GE(focus.zoom, LocationHistory::MinZoom);
   EXPECT_LE(focus.zoom, LocationHistory::MaxZoom);
}

TEST(MapFocus, TightClusterUsesHighZoom)
{
   LocationHistory::LocationPointList points;
   points.push_back(MakePoint(50.1100, 8.6800));
   points.push_back(MakePoint(50.1101, 8.6801));

   LocationHistory::MapFocus focus{};
   LocationHistory::ComputeDensestFocus(points, 800, 600, focus);
   EXPECT_GE(focus.zoom, 14);
}

TEST(MapFocus, SpanFocusCopiesCenterAndFitsTightSpan)
{
   LocationHistory::MapFocus focus{};
   const LocationHistory::FocusResult result = LocationHistory::ComputeSpanFocus(
      50.11,
      8.68,
      50.1100,
      50.1101,
      8.6800,
      8.6801,
      800,
      600,
      focus);
   EXPECT_TRUE(LocationHistory::IsOk(result));
   EXPECT_NEAR(focus.latitude, 50.11, 1.0e-9);
   EXPECT_NEAR(focus.longitude, 8.68, 1.0e-9);
   EXPECT_GE(focus.zoom, 14);
   EXPECT_LE(focus.zoom, LocationHistory::MaxZoom);
}

TEST(MapFocus, WiderSpanUsesLowerZoom)
{
   LocationHistory::MapFocus tight{};
   const LocationHistory::FocusResult tightResult = LocationHistory::ComputeSpanFocus(
      50.11,
      8.68,
      50.1100,
      50.1101,
      8.6800,
      8.6801,
      800,
      600,
      tight);

   LocationHistory::MapFocus wide{};
   const LocationHistory::FocusResult wideResult = LocationHistory::ComputeSpanFocus(
      51.0,
      10.0,
      48.0,
      54.0,
      6.0,
      14.0,
      800,
      600,
      wide);

   EXPECT_TRUE(LocationHistory::IsOk(tightResult));
   EXPECT_TRUE(LocationHistory::IsOk(wideResult));
   EXPECT_GT(tight.zoom, wide.zoom);
}
