/*!
 *\file story_time_test.cpp
 *\brief Unit tests for story scrubber timestamp mapping
 */

#include "story_time.h"

#include <gtest/gtest.h>

#include "civil_time.h"
#include "location_data.h"
#include "location_point.h"

TEST(StoryTime, CutoffAtEndsOfRange)
{
   const int64_t minTimeMs = 1000;
   const int64_t maxTimeMs = 2000;
   EXPECT_EQ(LocationHistory::TimeCutoffFromSlider(minTimeMs, maxTimeMs, 0), minTimeMs);
   EXPECT_EQ(LocationHistory::TimeCutoffFromSlider(minTimeMs, maxTimeMs, LocationHistory::StorySliderMax), maxTimeMs);
}

TEST(StoryTime, EmptyRangeReturnsMin)
{
   EXPECT_EQ(LocationHistory::TimeCutoffFromSlider(50, 50, 400), 50);
   EXPECT_EQ(LocationHistory::SliderFromTimeCutoff(50, 50, 50), LocationHistory::StorySliderMax);
}

TEST(StoryTime, SliderRoundTripMidpoint)
{
   const int64_t minTimeMs = 0;
   const int64_t maxTimeMs = 100000;
   const int32_t sliderMid = LocationHistory::StorySliderMax / 2;
   const int64_t cutoff = LocationHistory::TimeCutoffFromSlider(minTimeMs, maxTimeMs, sliderMid);
   EXPECT_EQ(LocationHistory::SliderFromTimeCutoff(minTimeMs, maxTimeMs, cutoff), sliderMid);
}

TEST(LocationPoint, VisibleUntilUsesStartTime)
{
   LocationHistory::LocationPoint point{};
   point.unixTimeMs = 500;
   point.endUnixTimeMs = 800;
   EXPECT_TRUE(LocationHistory::PointVisibleUntil(point, 500));
   EXPECT_FALSE(LocationHistory::PointVisibleUntil(point, 499));
   EXPECT_TRUE(LocationHistory::PointHasDuration(point));
}

TEST(StoryTime, PointIsOnCivilDateUsesLocalOffset)
{
   LocationHistory::LocationPoint point{};
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-15T08:30:00.000+01:00", point.unixTimeMs, point.utcOffsetMinutes)));
   EXPECT_TRUE(LocationHistory::PointIsOnCivilDate(point, 2020, 1, 15));
   EXPECT_FALSE(LocationHistory::PointIsOnCivilDate(point, 2020, 1, 16));
}

TEST(StoryTime, CollectPointsOnDateKeepsOnlyThatDay)
{
   LocationHistory::LocationPoint jan15{};
   LocationHistory::LocationPoint jan18{};
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-15T08:30:00.000+01:00", jan15.unixTimeMs, jan15.utcOffsetMinutes)));
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-18T19:00:00.000+01:00", jan18.unixTimeMs, jan18.utcOffsetMinutes)));
   jan15.latitude = 50.1;
   jan18.latitude = 50.2;

   LocationHistory::LocationPointList input;
   input.push_back(jan15);
   input.push_back(jan18);

   LocationHistory::LocationPointList output;
   LocationHistory::CollectPointsOnDate(input, 2020, 1, 15, output);
   ASSERT_EQ(output.size(), 1u);
   EXPECT_NEAR(output[0].latitude, 50.1, 1.0e-9);

   LocationHistory::CollectPointsOnDate(input, 2020, 1, 16, output);
   EXPECT_TRUE(output.empty());
}

TEST(StoryTime, CollectPointsFromDateKeepsStartDayAndLater)
{
   LocationHistory::LocationPoint jan15{};
   LocationHistory::LocationPoint jan16{};
   LocationHistory::LocationPoint jan18{};
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-15T08:30:00.000+01:00", jan15.unixTimeMs, jan15.utcOffsetMinutes)));
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-16T12:00:00.000+01:00", jan16.unixTimeMs, jan16.utcOffsetMinutes)));
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-18T19:00:00.000+01:00", jan18.unixTimeMs, jan18.utcOffsetMinutes)));
   jan15.latitude = 50.1;
   jan16.latitude = 50.15;
   jan18.latitude = 50.2;

   LocationHistory::LocationPointList input;
   input.push_back(jan15);
   input.push_back(jan16);
   input.push_back(jan18);

   LocationHistory::LocationPointList output;
   LocationHistory::CollectPointsFromDate(input, 2020, 1, 16, output);
   ASSERT_EQ(output.size(), 2u);
   EXPECT_NEAR(output[0].latitude, 50.15, 1.0e-9);
   EXPECT_NEAR(output[1].latitude, 50.2, 1.0e-9);
}

TEST(StoryTime, LastTimeOnCivilDateUsesEndTime)
{
   LocationHistory::LocationPoint jan15{};
   LocationHistory::LocationPoint jan18{};
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-15T08:30:00.000+01:00", jan15.unixTimeMs, jan15.utcOffsetMinutes)));
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-15T10:00:00.000+01:00", jan15.endUnixTimeMs, jan15.utcOffsetMinutes)));
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-18T19:00:00.000+01:00", jan18.unixTimeMs, jan18.utcOffsetMinutes)));
   jan18.endUnixTimeMs = jan18.unixTimeMs;

   LocationHistory::LocationPointList input;
   input.push_back(jan15);
   input.push_back(jan18);

   EXPECT_EQ(LocationHistory::LastTimeOnCivilDate(input, 2020, 1, 15), jan15.endUnixTimeMs);
   EXPECT_EQ(LocationHistory::LastTimeOnCivilDate(input, 2020, 1, 16), 0);
}
