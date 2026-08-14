/*!
 *\file civil_time.h
 *\brief Civil date/time conversion helpers for local Timeline timestamps
 */

#ifndef CIVIL_TIME_H
#define CIVIL_TIME_H

#include <cstdint>
#include <string>
#include <string_view>

#include "load_result.h"

namespace LocationHistory
{
   struct CivilDateTime
   {
      int32_t year;
      int32_t month;
      int32_t day;
      int32_t hour;
      int32_t minute;
      int32_t second;
      int32_t millisecond;
   };

   /*!
    *\brief Converts a civil date/time plus UTC offset to Unix milliseconds
    *
    *\param[in] dateTime Local civil date and time
    *\param[in] utcOffsetMinutes Offset east of UTC in minutes
    */
   int64_t CivilToUnixMs(const CivilDateTime& dateTime, int32_t utcOffsetMinutes);

   /*!
    *\brief Converts Unix milliseconds plus UTC offset to local civil date/time
    *
    *\param[in] unixTimeMs Unix time in milliseconds
    *\param[in] utcOffsetMinutes Offset east of UTC in minutes
    *\param[out] dateTime Local civil date and time
    */
   void UnixMsToCivil(int64_t unixTimeMs, int32_t utcOffsetMinutes, CivilDateTime& dateTime);

   /*!
    *\brief Returns the Monday-based weekday of a local timestamp (Monday = 0)
    *
    *\param[in] unixTimeMs Unix time in milliseconds
    *\param[in] utcOffsetMinutes Offset east of UTC in minutes
    */
   int32_t MondayBasedWeekday(int64_t unixTimeMs, int32_t utcOffsetMinutes);

   /*!
    *\brief Returns the local minute of day in the range 0..1439
    *
    *\param[in] unixTimeMs Unix time in milliseconds
    *\param[in] utcOffsetMinutes Offset east of UTC in minutes
    */
   int32_t MinuteOfDay(int64_t unixTimeMs, int32_t utcOffsetMinutes);

   /*!
    *\brief Formats a local timestamp as YYYY-MM-DD HH:MM:SS
    *
    *\param[in] unixTimeMs Unix time in milliseconds
    *\param[in] utcOffsetMinutes Offset east of UTC in minutes
    *\param[out] output Formatted local time string
    */
   void FormatLocalTime(int64_t unixTimeMs, int32_t utcOffsetMinutes, std::string& output);

   /*!
    *\brief Parses an ISO-8601 timestamp with numeric offset or Z
    *
    *\param[in] text Timestamp text from Google Timeline JSON
    *\param[out] unixTimeMs Unix time in milliseconds
    *\param[out] utcOffsetMinutes Offset east of UTC in minutes
    */
   ParseResult ParseIso8601(std::string_view text, int64_t& unixTimeMs, int32_t& utcOffsetMinutes);
} // namespace LocationHistory

#endif // CIVIL_TIME_H
