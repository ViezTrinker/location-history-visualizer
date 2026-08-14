/*!
 *\file json_loader_test.cpp
 *\brief Unit tests for Timeline JSON parsing
 */

#include "json_loader.h"

#include <string>

#include <gtest/gtest.h>

#include "civil_time.h"
#include "load_result.h"
#include "location_data.h"
#include "location_point.h"

TEST(JsonLoader, ParseLatLngAcceptsDegreeStrings)
{
   double latitude = 0.0;
   double longitude = 0.0;
   const LocationHistory::ParseResult result =
      LocationHistory::ParseLatLng("50.1109°, 8.6821°", latitude, longitude);
   EXPECT_TRUE(LocationHistory::IsOk(result));
   EXPECT_NEAR(latitude, 50.1109, 1.0e-6);
   EXPECT_NEAR(longitude, 8.6821, 1.0e-6);
}

TEST(JsonLoader, ParseLatLngRejectsMissingComma)
{
   double latitude = 0.0;
   double longitude = 0.0;
   const LocationHistory::ParseResult result = LocationHistory::ParseLatLng("50.1109 8.6821", latitude, longitude);
   EXPECT_TRUE(LocationHistory::IsErr(result));
}

TEST(JsonLoader, ParseIso8601WithOffset)
{
   int64_t unixTimeMs = 0;
   int32_t utcOffsetMinutes = 0;
   const LocationHistory::ParseResult result =
      LocationHistory::ParseIso8601("2020-01-15T08:30:00.000+01:00", unixTimeMs, utcOffsetMinutes);
   EXPECT_TRUE(LocationHistory::IsOk(result));
   EXPECT_EQ(utcOffsetMinutes, 60);

   LocationHistory::CivilDateTime dateTime{};
   LocationHistory::UnixMsToCivil(unixTimeMs, utcOffsetMinutes, dateTime);
   EXPECT_EQ(dateTime.year, 2020);
   EXPECT_EQ(dateTime.month, 1);
   EXPECT_EQ(dateTime.day, 15);
   EXPECT_EQ(dateTime.hour, 8);
   EXPECT_EQ(dateTime.minute, 30);
}

TEST(JsonLoader, LoadFixtureExtractsAllSources)
{
   LocationHistory::LocationPointList points;
   const std::string fixturePath = std::string(TEST_FIXTURE_DIR) + "/sample_timeline.json";
   const LocationHistory::LoadResult result = LocationHistory::LoadFromFile(fixturePath, points);
   EXPECT_TRUE(LocationHistory::IsOk(result));
   EXPECT_EQ(points.size(), 5u);

   int32_t timelineCount = 0;
   int32_t visitCount = 0;
   int32_t rawCount = 0;
   for (size_t index = 0; index < points.size(); ++index)
   {
      if (points[index].source == LocationHistory::PointSource::TimelinePath)
      {
         timelineCount += 1;
      }
      if (points[index].source == LocationHistory::PointSource::Visit)
      {
         visitCount += 1;
      }
      if (points[index].source == LocationHistory::PointSource::RawPosition)
      {
         rawCount += 1;
      }
   }
   EXPECT_EQ(timelineCount, 3);
   EXPECT_EQ(visitCount, 1);
   EXPECT_EQ(rawCount, 1);
}

TEST(JsonLoader, MissingFileReturnsFileNotFound)
{
   LocationHistory::LocationPointList points;
   const LocationHistory::LoadResult result = LocationHistory::LoadFromFile("this_file_does_not_exist.json", points);
   EXPECT_EQ(result, LocationHistory::LoadResult::FileNotFound);
}

TEST(JsonLoader, EmptyObjectReturnsNoPoints)
{
   LocationHistory::LocationPointList points;
   const LocationHistory::LoadResult result = LocationHistory::LoadFromString("{}", points);
   EXPECT_EQ(result, LocationHistory::LoadResult::NoPoints);
}

TEST(JsonLoader, InvalidJsonReturnsError)
{
   LocationHistory::LocationPointList points;
   const LocationHistory::LoadResult result = LocationHistory::LoadFromString("{ not json", points);
   EXPECT_EQ(result, LocationHistory::LoadResult::InvalidJson);
}

TEST(JsonLoader, EmptyStringReturnsInvalidJson)
{
   LocationHistory::LocationPointList points;
   const LocationHistory::LoadResult result = LocationHistory::LoadFromString("", points);
   EXPECT_EQ(result, LocationHistory::LoadResult::InvalidJson);
}
