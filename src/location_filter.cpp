/*!
 *\file location_filter.cpp
 *\brief Date, weekday, and time-of-day filters for location points
 */

#include "location_filter.h"

#include <cstdint>

#include "civil_time.h"
#include "location_data.h"

namespace LocationHistory
{
   namespace
   {
      int32_t PackDate(const int32_t year, const int32_t month, const int32_t day)
      {
         return year * 10000 + month * 100 + day;
      }
   } // namespace

   FilterSettings MakePassThroughFilter(void)
   {
      FilterSettings settings{};
      settings.dateFilter = FilterActive::No;
      settings.fromYear = 1970;
      settings.fromMonth = 1;
      settings.fromDay = 1;
      settings.toYear = 9999;
      settings.toMonth = 12;
      settings.toDay = 31;
      settings.weekdayMask = WeekdayMaskAll;
      settings.timeFilter = FilterActive::No;
      settings.fromMinuteOfDay = 0;
      settings.toMinuteOfDay = 1439;
      return settings;
   }

   bool PointMatchesFilter(const LocationPoint& point, const FilterSettings& settings)
   {
      if (settings.dateFilter == FilterActive::Yes)
      {
         CivilDateTime dateTime{};
         UnixMsToCivil(point.unixTimeMs, point.utcOffsetMinutes, dateTime);
         const int32_t packedDate = PackDate(dateTime.year, dateTime.month, dateTime.day);
         const int32_t packedFrom = PackDate(settings.fromYear, settings.fromMonth, settings.fromDay);
         const int32_t packedTo = PackDate(settings.toYear, settings.toMonth, settings.toDay);
         if (packedDate < packedFrom)
         {
            return false;
         }
         if (packedDate > packedTo)
         {
            return false;
         }
      }

      const uint8_t weekdayBit = static_cast<uint8_t>(
         1u << static_cast<uint8_t>(MondayBasedWeekday(point.unixTimeMs, point.utcOffsetMinutes)));
      if ((settings.weekdayMask & weekdayBit) == 0)
      {
         return false;
      }

      if (settings.timeFilter == FilterActive::Yes)
      {
         const int32_t minuteOfDay = MinuteOfDay(point.unixTimeMs, point.utcOffsetMinutes);
         if (settings.fromMinuteOfDay <= settings.toMinuteOfDay)
         {
            if (minuteOfDay < settings.fromMinuteOfDay)
            {
               return false;
            }
            if (minuteOfDay > settings.toMinuteOfDay)
            {
               return false;
            }
         }
         else
         {
            if ((minuteOfDay < settings.fromMinuteOfDay) && (minuteOfDay > settings.toMinuteOfDay))
            {
               return false;
            }
         }
      }

      return true;
   }

   void ApplyFilter(const LocationPointList& input, const FilterSettings& settings, LocationPointList& output)
   {
      output.clear();
      output.reserve(input.size());
      for (size_t index = 0; index < input.size(); ++index)
      {
         if (PointMatchesFilter(input[index], settings))
         {
            output.push_back(input[index]);
         }
      }
   }
} // namespace LocationHistory
