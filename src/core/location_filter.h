/*!
 *\file location_filter.h
 *\brief Date, weekday, and time-of-day filters for location points
 */

#ifndef LOCATION_FILTER_H
#define LOCATION_FILTER_H

#include <cstdint>

#include "location_data.h"

namespace LocationHistory
{
   enum class FilterActive : bool
   {
      No = false,
      Yes = true
   };

   enum class Weekday : uint8_t
   {
      Monday = 0,
      Tuesday = 1,
      Wednesday = 2,
      Thursday = 3,
      Friday = 4,
      Saturday = 5,
      Sunday = 6
   };

   inline constexpr uint8_t WeekdayMaskAll = 127;

   /*!
    *\brief Returns the bit flag for a Monday-based weekday
    *
    *\param[in] weekday Weekday to convert
    */
   inline constexpr uint8_t WeekdayFlag(const Weekday weekday)
   {
      return static_cast<uint8_t>(1u << static_cast<uint8_t>(weekday));
   }

   struct FilterSettings
   {
      FilterActive dateFilter;
      int32_t fromYear;
      int32_t fromMonth;
      int32_t fromDay;
      int32_t toYear;
      int32_t toMonth;
      int32_t toDay;
      uint8_t weekdayMask;
      FilterActive timeFilter;
      int32_t fromMinuteOfDay;
      int32_t toMinuteOfDay;
   };

   /*!
    *\brief Returns filter settings that pass every point
    */
   FilterSettings MakePassThroughFilter(void);

   /*!
    *\brief Applies date, weekday, and time filters to a point list
    *
    *\param[in] input Unfiltered location points
    *\param[in] settings Filter configuration
    *\param[out] output Points that match the filter
    */
   void ApplyFilter(const LocationPointList& input, const FilterSettings& settings, LocationPointList& output);

   /*!
    *\brief Returns true if a single point matches the filter
    *
    *\param[in] point Location point to test
    *\param[in] settings Filter configuration
    */
   bool PointMatchesFilter(const LocationPoint& point, const FilterSettings& settings);
} // namespace LocationHistory

#endif // LOCATION_FILTER_H
