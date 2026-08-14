/*!
 *\file location_filter_test.cpp
 *\brief Unit tests for date, weekday, and time filters
 */

#include "location_filter.h"

#include <string_view>

#include <gtest/gtest.h>

#include "civil_time.h"
#include "location_data.h"
#include "location_point.h"

namespace
{
   LocationHistory::LocationPoint MakePoint(const std::string_view timestamp, const double latitude, const double longitude)
   {
      LocationHistory::LocationPoint point{};
      point.latitude = latitude;
      point.longitude = longitude;
      point.source = LocationHistory::PointSource::TimelinePath;
      const LocationHistory::ParseResult parseResult =
         LocationHistory::ParseIso8601(timestamp, point.unixTimeMs, point.utcOffsetMinutes);
      EXPECT_TRUE(LocationHistory::IsOk(parseResult));
      return point;
   }
} // namespace

TEST(LocationFilter, PassThroughKeepsAllPoints)
{
   LocationHistory::LocationPointList input;
   input.push_back(MakePoint("2020-01-15T08:30:00.000+01:00", 50.1, 8.6));
   input.push_back(MakePoint("2020-01-18T19:00:00.000+01:00", 50.2, 8.7));

   LocationHistory::LocationPointList output;
   LocationHistory::ApplyFilter(input, LocationHistory::MakePassThroughFilter(), output);
   EXPECT_EQ(output.size(), 2u);
}

TEST(LocationFilter, DateRangeExcludesLaterDay)
{
   LocationHistory::LocationPointList input;
   input.push_back(MakePoint("2020-01-15T08:30:00.000+01:00", 50.1, 8.6));
   input.push_back(MakePoint("2020-01-18T19:00:00.000+01:00", 50.2, 8.7));

   LocationHistory::FilterSettings settings = LocationHistory::MakePassThroughFilter();
   settings.dateFilter = LocationHistory::FilterActive::Yes;
   settings.fromYear = 2020;
   settings.fromMonth = 1;
   settings.fromDay = 15;
   settings.toYear = 2020;
   settings.toMonth = 1;
   settings.toDay = 15;

   LocationHistory::LocationPointList output;
   LocationHistory::ApplyFilter(input, settings, output);
   EXPECT_EQ(output.size(), 1u);
   EXPECT_NEAR(output[0].latitude, 50.1, 1.0e-9);
}

TEST(LocationFilter, WeekdayMaskKeepsWednesday)
{
   const LocationHistory::LocationPoint wednesday = MakePoint("2020-01-15T08:30:00.000+01:00", 50.1, 8.6);
   const LocationHistory::LocationPoint saturday = MakePoint("2020-01-18T19:00:00.000+01:00", 50.2, 8.7);
   EXPECT_EQ(LocationHistory::MondayBasedWeekday(wednesday.unixTimeMs, wednesday.utcOffsetMinutes), 2);
   EXPECT_EQ(LocationHistory::MondayBasedWeekday(saturday.unixTimeMs, saturday.utcOffsetMinutes), 5);

   LocationHistory::LocationPointList input;
   input.push_back(wednesday);
   input.push_back(saturday);

   LocationHistory::FilterSettings settings = LocationHistory::MakePassThroughFilter();
   settings.weekdayMask = LocationHistory::WeekdayFlag(LocationHistory::Weekday::Wednesday);

   LocationHistory::LocationPointList output;
   LocationHistory::ApplyFilter(input, settings, output);
   EXPECT_EQ(output.size(), 1u);
   EXPECT_NEAR(output[0].latitude, 50.1, 1.0e-9);
}

TEST(LocationFilter, TimeWindowKeepsMorning)
{
   LocationHistory::LocationPointList input;
   input.push_back(MakePoint("2020-01-15T08:30:00.000+01:00", 50.1, 8.6));
   input.push_back(MakePoint("2020-01-18T19:00:00.000+01:00", 50.2, 8.7));

   LocationHistory::FilterSettings settings = LocationHistory::MakePassThroughFilter();
   settings.timeFilter = LocationHistory::FilterActive::Yes;
   settings.fromMinuteOfDay = 8 * 60;
   settings.toMinuteOfDay = 9 * 60;

   LocationHistory::LocationPointList output;
   LocationHistory::ApplyFilter(input, settings, output);
   EXPECT_EQ(output.size(), 1u);
   EXPECT_NEAR(output[0].latitude, 50.1, 1.0e-9);
}
