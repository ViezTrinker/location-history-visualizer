/*!
 *\file story_time.cpp
 *\brief Mapping between the story scrubber and timestamps
 */

#include "story_time.h"

#include <cstdint>

#include "civil_time.h"

namespace LocationHistory
{
   int64_t TimeCutoffFromSlider(const int64_t minTimeMs, const int64_t maxTimeMs, const int32_t sliderValue)
   {
      if (maxTimeMs <= minTimeMs)
      {
         return minTimeMs;
      }

      int32_t clampedSlider = sliderValue;
      if (clampedSlider < 0)
      {
         clampedSlider = 0;
      }
      if (clampedSlider > StorySliderMax)
      {
         clampedSlider = StorySliderMax;
      }

      const auto span = static_cast<double>(maxTimeMs - minTimeMs);
      const double ratio = static_cast<double>(clampedSlider) / static_cast<double>(StorySliderMax);
      return minTimeMs + static_cast<int64_t>(span * ratio);
   }

   int32_t SliderFromTimeCutoff(const int64_t minTimeMs, const int64_t maxTimeMs, const int64_t cutoffMs)
   {
      if (maxTimeMs <= minTimeMs)
      {
         return StorySliderMax;
      }
      if (cutoffMs <= minTimeMs)
      {
         return 0;
      }
      if (cutoffMs >= maxTimeMs)
      {
         return StorySliderMax;
      }

      const auto span = static_cast<double>(maxTimeMs - minTimeMs);
      const double ratio = static_cast<double>(cutoffMs - minTimeMs) / span;
      const auto sliderValue = static_cast<int32_t>(ratio * static_cast<double>(StorySliderMax) + 0.5);
      if (sliderValue < 0)
      {
         return 0;
      }
      if (sliderValue > StorySliderMax)
      {
         return StorySliderMax;
      }
      return sliderValue;
   }

   bool PointIsOnCivilDate(
      const LocationPoint& point,
      const int32_t year,
      const int32_t month,
      const int32_t day)
   {
      CivilDateTime dateTime{};
      UnixMsToCivil(point.unixTimeMs, point.utcOffsetMinutes, dateTime);
      if (dateTime.year != year)
      {
         return false;
      }
      if (dateTime.month != month)
      {
         return false;
      }
      return dateTime.day == day;
   }

   bool PointIsOnOrAfterCivilDate(
      const LocationPoint& point,
      const int32_t year,
      const int32_t month,
      const int32_t day)
   {
      CivilDateTime dateTime{};
      UnixMsToCivil(point.unixTimeMs, point.utcOffsetMinutes, dateTime);
      if (dateTime.year > year)
      {
         return true;
      }
      if (dateTime.year < year)
      {
         return false;
      }
      if (dateTime.month > month)
      {
         return true;
      }
      if (dateTime.month < month)
      {
         return false;
      }
      return dateTime.day >= day;
   }

   void CollectPointsOnDate(
      const LocationPointList& input,
      const int32_t year,
      const int32_t month,
      const int32_t day,
      LocationPointList& output)
   {
      output.clear();
      output.reserve(input.size());
      for (size_t index = 0; index < input.size(); ++index)
      {
         if (PointIsOnCivilDate(input[index], year, month, day))
         {
            output.push_back(input[index]);
         }
      }
   }

   void CollectPointsFromDate(
      const LocationPointList& input,
      const int32_t year,
      const int32_t month,
      const int32_t day,
      LocationPointList& output)
   {
      output.clear();
      output.reserve(input.size());
      for (size_t index = 0; index < input.size(); ++index)
      {
         if (PointIsOnOrAfterCivilDate(input[index], year, month, day))
         {
            output.push_back(input[index]);
         }
      }
   }

   int64_t LastTimeOnCivilDate(
      const LocationPointList& input,
      const int32_t year,
      const int32_t month,
      const int32_t day)
   {
      int64_t lastTimeMs = 0;
      for (size_t index = 0; index < input.size(); ++index)
      {
         const LocationPoint& point = input[index];
         if (!PointIsOnCivilDate(point, year, month, day))
         {
            continue;
         }

         int64_t pointTimeMs = point.endUnixTimeMs;
         if (pointTimeMs < point.unixTimeMs)
         {
            pointTimeMs = point.unixTimeMs;
         }
         if (pointTimeMs > lastTimeMs)
         {
            lastTimeMs = pointTimeMs;
         }
      }
      return lastTimeMs;
   }
} // namespace LocationHistory
