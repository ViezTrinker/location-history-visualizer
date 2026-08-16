/*!
 *\file json_loader_test.cpp
 *\brief Unit tests for Timeline JSON parsing
 */

#include "json_loader.h"

#include <string>

#include <gtest/gtest.h>

#include "civil_time.h"
#include "load_observer.h"
#include "load_result.h"
#include "location_data.h"
#include "location_point.h"

namespace
{
   class RecordingObserver : public LocationHistory::LoadObserver
   {
      public:
         void OnProgress(const int64_t bytesRead, const int64_t bytesTotal) override
         {
            lastBytesRead = bytesRead;
            lastBytesTotal = bytesTotal;
            reportCount += 1;
         }

         bool IsCancelled(void) const override
         {
            return false;
         }

         int32_t reportCount = 0;
         int64_t lastBytesRead = -1;
         int64_t lastBytesTotal = -1;
   };

   class CancelImmediatelyObserver : public LocationHistory::LoadObserver
   {
      public:
         void OnProgress(const int64_t bytesRead, const int64_t bytesTotal) override
         {
            (void)bytesRead;
            (void)bytesTotal;
         }

         bool IsCancelled(void) const override
         {
            return true;
         }
   };

   class CancelAfterFirstProgressObserver : public LocationHistory::LoadObserver
   {
      public:
         void OnProgress(const int64_t bytesRead, const int64_t bytesTotal) override
         {
            (void)bytesRead;
            (void)bytesTotal;
            _gotProgress = true;
         }

         bool IsCancelled(void) const override
         {
            return _gotProgress;
         }

      private:
         bool _gotProgress = false;
   };
} // namespace

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

TEST(JsonLoader, FormatIso8601RoundTripsWithOffset)
{
   int64_t unixTimeMs = 0;
   int32_t utcOffsetMinutes = 0;
   ASSERT_TRUE(LocationHistory::IsOk(
      LocationHistory::ParseIso8601("2020-01-15T08:30:00.000+01:00", unixTimeMs, utcOffsetMinutes)));

   std::string formatted;
   LocationHistory::FormatIso8601(unixTimeMs, utcOffsetMinutes, formatted);
   EXPECT_EQ(formatted, "2020-01-15T08:30:00.000+01:00");

   int64_t parsedAgain = 0;
   int32_t parsedOffset = 0;
   ASSERT_TRUE(LocationHistory::IsOk(LocationHistory::ParseIso8601(formatted, parsedAgain, parsedOffset)));
   EXPECT_EQ(parsedAgain, unixTimeMs);
   EXPECT_EQ(parsedOffset, utcOffsetMinutes);
}

TEST(JsonLoader, FormatIso8601FormatsNegativeOffset)
{
   LocationHistory::CivilDateTime dateTime{};
   dateTime.year = 2020;
   dateTime.month = 6;
   dateTime.day = 1;
   dateTime.hour = 12;
   dateTime.minute = 0;
   dateTime.second = 0;
   dateTime.millisecond = 0;
   const int32_t utcOffsetMinutes = -300;
   const int64_t unixTimeMs = LocationHistory::CivilToUnixMs(dateTime, utcOffsetMinutes);

   std::string formatted;
   LocationHistory::FormatIso8601(unixTimeMs, utcOffsetMinutes, formatted);
   EXPECT_EQ(formatted, "2020-06-01T12:00:00.000-05:00");
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

TEST(JsonLoader, TimelinePathsShareAnIdPerSegment)
{
   LocationHistory::LocationPointList points;
   const std::string fixturePath = std::string(TEST_FIXTURE_DIR) + "/sample_timeline.json";
   ASSERT_TRUE(LocationHistory::IsOk(LocationHistory::LoadFromFile(fixturePath, points)));

   int32_t firstPathId = LocationHistory::NoPathId;
   int32_t firstPathCount = 0;
   int32_t secondPathId = LocationHistory::NoPathId;
   for (size_t index = 0; index < points.size(); ++index)
   {
      if (points[index].source != LocationHistory::PointSource::TimelinePath)
      {
         continue;
      }
      if (firstPathCount == 0)
      {
         firstPathId = points[index].pathId;
      }
      if (points[index].pathId == firstPathId)
      {
         firstPathCount += 1;
      }
      else
      {
         secondPathId = points[index].pathId;
      }
   }
   EXPECT_NE(firstPathId, LocationHistory::NoPathId);
   EXPECT_EQ(firstPathCount, 2);
   EXPECT_NE(secondPathId, LocationHistory::NoPathId);
   EXPECT_NE(secondPathId, firstPathId);
}

TEST(JsonLoader, VisitKeepsSegmentDuration)
{
   LocationHistory::LocationPointList points;
   const std::string fixturePath = std::string(TEST_FIXTURE_DIR) + "/sample_timeline.json";
   ASSERT_TRUE(LocationHistory::IsOk(LocationHistory::LoadFromFile(fixturePath, points)));

   int32_t visitIndex = -1;
   for (size_t index = 0; index < points.size(); ++index)
   {
      if (points[index].source == LocationHistory::PointSource::Visit)
      {
         visitIndex = static_cast<int32_t>(index);
         break;
      }
   }
   ASSERT_GE(visitIndex, 0);
   const LocationHistory::LocationPoint& visit = points[static_cast<size_t>(visitIndex)];
   EXPECT_TRUE(LocationHistory::PointHasDuration(visit));
   EXPECT_EQ(visit.endUnixTimeMs - visit.unixTimeMs, 2 * 60 * 60 * 1000);
   EXPECT_EQ(visit.pathId, LocationHistory::NoPathId);
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

TEST(JsonLoader, ReportsProgressForFileLoad)
{
   LocationHistory::LocationPointList points;
   const std::string fixturePath = std::string(TEST_FIXTURE_DIR) + "/sample_timeline.json";
   RecordingObserver observer;
   const LocationHistory::LoadResult result =
      LocationHistory::LoadFromFile(fixturePath, points, &observer);
   EXPECT_TRUE(LocationHistory::IsOk(result));
   EXPECT_GE(observer.reportCount, 2);
   EXPECT_EQ(observer.lastBytesRead, observer.lastBytesTotal);
   EXPECT_GT(observer.lastBytesTotal, 0);
}

TEST(JsonLoader, CancelBeforeParseReturnsCancelled)
{
   LocationHistory::LocationPointList points;
   CancelImmediatelyObserver observer;
   const LocationHistory::LoadResult result = LocationHistory::LoadFromString("{}", points, &observer);
   EXPECT_EQ(result, LocationHistory::LoadResult::Cancelled);
   EXPECT_TRUE(points.empty());
}

TEST(JsonLoader, CancelAfterFirstProgressReturnsCancelled)
{
   LocationHistory::LocationPointList points;
   CancelAfterFirstProgressObserver observer;
   const LocationHistory::LoadResult result = LocationHistory::LoadFromString("{}", points, &observer);
   EXPECT_EQ(result, LocationHistory::LoadResult::Cancelled);
}
