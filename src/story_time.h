/*!
 *\file story_time.h
 *\brief Mapping between the story scrubber and timestamps
 */

#ifndef STORY_TIME_H
#define STORY_TIME_H

#include <cstdint>
#include <limits>

#include "location_data.h"

namespace LocationHistory
{
   inline constexpr int32_t StorySliderMax = 1000;
   inline constexpr int64_t ShowAllUntilTimeMs = std::numeric_limits<int64_t>::max();

   /*!
    *\brief Maps a scrubber position onto a timestamp between min and max
    *
    *\param[in] minTimeMs Start of the filtered range
    *\param[in] maxTimeMs End of the filtered range
    *\param[in] sliderValue Scrubber value from 0 to StorySliderMax
    */
   int64_t TimeCutoffFromSlider(int64_t minTimeMs, int64_t maxTimeMs, int32_t sliderValue);

   /*!
    *\brief Maps a timestamp back onto a scrubber position
    *
    *\param[in] minTimeMs Start of the filtered range
    *\param[in] maxTimeMs End of the filtered range
    *\param[in] cutoffMs Timestamp to convert
    */
   int32_t SliderFromTimeCutoff(int64_t minTimeMs, int64_t maxTimeMs, int64_t cutoffMs);

   /*!
    *\brief Returns true if the sample's local start date matches year/month/day
    *
    *\param[in] point Location sample
    *\param[in] year Civil year
    *\param[in] month Civil month 1..12
    *\param[in] day Civil day of month
    */
   bool PointIsOnCivilDate(const LocationPoint& point, int32_t year, int32_t month, int32_t day);

   /*!
    *\brief Returns true if the sample's local start date is on or after year/month/day
    *
    *\param[in] point Location sample
    *\param[in] year Civil year
    *\param[in] month Civil month 1..12
    *\param[in] day Civil day of month
    */
   bool PointIsOnOrAfterCivilDate(const LocationPoint& point, int32_t year, int32_t month, int32_t day);

   /*!
    *\brief Copies samples whose local start date matches year/month/day
    *
    *\param[in] input Location samples
    *\param[in] year Civil year
    *\param[in] month Civil month 1..12
    *\param[in] day Civil day of month
    *\param[out] output Samples on that local date, in input order
    */
   void CollectPointsOnDate(
      const LocationPointList& input,
      int32_t year,
      int32_t month,
      int32_t day,
      LocationPointList& output);

   /*!
    *\brief Copies samples on or after the given local date
    *
    *\param[in] input Location samples
    *\param[in] year Civil year
    *\param[in] month Civil month 1..12
    *\param[in] day Civil day of month
    *\param[out] output Samples from that date onward, in input order
    */
   void CollectPointsFromDate(
      const LocationPointList& input,
      int32_t year,
      int32_t month,
      int32_t day,
      LocationPointList& output);

   /*!
    *\brief Returns the latest start or end time of samples on the given local date
    *
    *\param[in] input Location samples
    *\param[in] year Civil year
    *\param[in] month Civil month 1..12
    *\param[in] day Civil day of month
    */
   int64_t LastTimeOnCivilDate(
      const LocationPointList& input,
      int32_t year,
      int32_t month,
      int32_t day);
} // namespace LocationHistory

#endif // STORY_TIME_H
