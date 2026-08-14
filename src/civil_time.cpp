/*!
 *\file civil_time.cpp
 *\brief Civil date/time conversion helpers for local Timeline timestamps
 */

#include "civil_time.h"

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace LocationHistory
{
   namespace
   {
      inline constexpr int64_t MillisecondsPerSecond = 1000;
      inline constexpr int64_t SecondsPerMinute = 60;
      inline constexpr int64_t MinutesPerHour = 60;
      inline constexpr int64_t HoursPerDay = 24;
      inline constexpr int64_t MinutesPerDay = 1440;
      inline constexpr int64_t MillisecondsPerMinute = MillisecondsPerSecond * SecondsPerMinute;
      inline constexpr int64_t MillisecondsPerDay = MillisecondsPerMinute * MinutesPerDay;
      inline constexpr int32_t IsoDateTimeLength = 19;
      inline constexpr int32_t MondayBasedThursday = 3;

      int32_t ParseDigits(std::string_view text, const size_t offset, const size_t digitCount, int32_t& value)
      {
         if ((offset + digitCount) > text.size())
         {
            return 0;
         }

         const std::string_view token = text.substr(offset, digitCount);
         int32_t parsedValue = 0;
         const char* pBegin = token.data();
         const char* pEnd = pBegin + token.size();
         const auto parseResult = std::from_chars(pBegin, pEnd, parsedValue);
         if (parseResult.ec != std::errc())
         {
            return 0;
         }
         if (parseResult.ptr != pEnd)
         {
            return 0;
         }

         value = parsedValue;
         return 1;
      }

      int64_t DaysFromCivil(int32_t year, const int32_t month, const int32_t day)
      {
         if (month <= 2)
         {
            year -= 1;
         }

         const int32_t era = (year >= 0 ? year : year - 399) / 400;
         const auto yearOfEra = static_cast<uint32_t>(year - era * 400);
         const int32_t shiftedMonth = month + ((month > 2) ? -3 : 9);
         const auto dayOfYear = static_cast<uint32_t>((153 * shiftedMonth + 2) / 5 + day - 1);
         const auto dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
         return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(dayOfEra) - 719468;
      }

      void CivilFromDays(int64_t days, int32_t& year, int32_t& month, int32_t& day)
      {
         days += 719468;
         const int32_t era = static_cast<int32_t>((days >= 0 ? days : days - 146096) / 146097);
         const auto dayOfEra = static_cast<uint32_t>(days - static_cast<int64_t>(era) * 146097);
         const auto yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
         year = static_cast<int32_t>(yearOfEra) + era * 400;
         const auto dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
         const auto monthPrime = (5 * dayOfYear + 2) / 153;
         day = static_cast<int32_t>(dayOfYear - (153 * monthPrime + 2) / 5 + 1);
         month = static_cast<int32_t>(monthPrime) + ((monthPrime < 10) ? 3 : -9);
         if (month <= 2)
         {
            year += 1;
         }
      }

      int64_t LocalUnixMs(const int64_t unixTimeMs, const int32_t utcOffsetMinutes)
      {
         return unixTimeMs + static_cast<int64_t>(utcOffsetMinutes) * MillisecondsPerMinute;
      }

      void AppendFourDigits(std::string& output, const int32_t value)
      {
         output.push_back(static_cast<char>('0' + ((value / 1000) % 10)));
         output.push_back(static_cast<char>('0' + ((value / 100) % 10)));
         output.push_back(static_cast<char>('0' + ((value / 10) % 10)));
         output.push_back(static_cast<char>('0' + (value % 10)));
      }

      void AppendTwoDigits(std::string& output, const int32_t value)
      {
         output.push_back(static_cast<char>('0' + ((value / 10) % 10)));
         output.push_back(static_cast<char>('0' + (value % 10)));
      }
   } // namespace

   int64_t CivilToUnixMs(const CivilDateTime& dateTime, const int32_t utcOffsetMinutes)
   {
      const int64_t days = DaysFromCivil(dateTime.year, dateTime.month, dateTime.day);
      const int64_t localMs =
         days * MillisecondsPerDay +
         static_cast<int64_t>(dateTime.hour) * MinutesPerHour * MillisecondsPerMinute +
         static_cast<int64_t>(dateTime.minute) * MillisecondsPerMinute +
         static_cast<int64_t>(dateTime.second) * MillisecondsPerSecond +
         static_cast<int64_t>(dateTime.millisecond);
      return localMs - static_cast<int64_t>(utcOffsetMinutes) * MillisecondsPerMinute;
   }

   void UnixMsToCivil(const int64_t unixTimeMs, const int32_t utcOffsetMinutes, CivilDateTime& dateTime)
   {
      const int64_t localMs = LocalUnixMs(unixTimeMs, utcOffsetMinutes);
      int64_t dayIndex = localMs / MillisecondsPerDay;
      int64_t remainingMs = localMs % MillisecondsPerDay;
      if (remainingMs < 0)
      {
         remainingMs += MillisecondsPerDay;
         dayIndex -= 1;
      }

      CivilFromDays(dayIndex, dateTime.year, dateTime.month, dateTime.day);
      dateTime.hour = static_cast<int32_t>(remainingMs / (MinutesPerHour * MillisecondsPerMinute));
      remainingMs %= (MinutesPerHour * MillisecondsPerMinute);
      dateTime.minute = static_cast<int32_t>(remainingMs / MillisecondsPerMinute);
      remainingMs %= MillisecondsPerMinute;
      dateTime.second = static_cast<int32_t>(remainingMs / MillisecondsPerSecond);
      dateTime.millisecond = static_cast<int32_t>(remainingMs % MillisecondsPerSecond);
   }

   int32_t MondayBasedWeekday(const int64_t unixTimeMs, const int32_t utcOffsetMinutes)
   {
      const int64_t localMs = LocalUnixMs(unixTimeMs, utcOffsetMinutes);
      int64_t dayIndex = localMs / MillisecondsPerDay;
      if ((localMs % MillisecondsPerDay) < 0)
      {
         dayIndex -= 1;
      }

      int32_t weekday = static_cast<int32_t>((dayIndex + MondayBasedThursday) % 7);
      if (weekday < 0)
      {
         weekday += 7;
      }
      return weekday;
   }

   int32_t MinuteOfDay(const int64_t unixTimeMs, const int32_t utcOffsetMinutes)
   {
      const int64_t localMs = LocalUnixMs(unixTimeMs, utcOffsetMinutes);
      int64_t remainingMs = localMs % MillisecondsPerDay;
      if (remainingMs < 0)
      {
         remainingMs += MillisecondsPerDay;
      }
      return static_cast<int32_t>(remainingMs / MillisecondsPerMinute);
   }

   void FormatLocalTime(const int64_t unixTimeMs, const int32_t utcOffsetMinutes, std::string& output)
   {
      CivilDateTime dateTime{};
      UnixMsToCivil(unixTimeMs, utcOffsetMinutes, dateTime);

      output.clear();
      output.reserve(19);
      AppendFourDigits(output, dateTime.year);
      output.push_back('-');
      AppendTwoDigits(output, dateTime.month);
      output.push_back('-');
      AppendTwoDigits(output, dateTime.day);
      output.push_back(' ');
      AppendTwoDigits(output, dateTime.hour);
      output.push_back(':');
      AppendTwoDigits(output, dateTime.minute);
      output.push_back(':');
      AppendTwoDigits(output, dateTime.second);
   }

   ParseResult ParseIso8601(const std::string_view text, int64_t& unixTimeMs, int32_t& utcOffsetMinutes)
   {
      if (text.size() < static_cast<size_t>(IsoDateTimeLength))
      {
         return ParseResult::InvalidFormat;
      }
      if ((text[4] != '-') || (text[7] != '-') || (text[10] != 'T') || (text[13] != ':') || (text[16] != ':'))
      {
         return ParseResult::InvalidFormat;
      }

      CivilDateTime dateTime{};
      if (ParseDigits(text, 0, 4, dateTime.year) == 0)
      {
         return ParseResult::InvalidFormat;
      }
      if (ParseDigits(text, 5, 2, dateTime.month) == 0)
      {
         return ParseResult::InvalidFormat;
      }
      if (ParseDigits(text, 8, 2, dateTime.day) == 0)
      {
         return ParseResult::InvalidFormat;
      }
      if (ParseDigits(text, 11, 2, dateTime.hour) == 0)
      {
         return ParseResult::InvalidFormat;
      }
      if (ParseDigits(text, 14, 2, dateTime.minute) == 0)
      {
         return ParseResult::InvalidFormat;
      }
      if (ParseDigits(text, 17, 2, dateTime.second) == 0)
      {
         return ParseResult::InvalidFormat;
      }

      dateTime.millisecond = 0;
      size_t offsetIndex = 19;
      if ((offsetIndex < text.size()) && (text[offsetIndex] == '.'))
      {
         offsetIndex += 1;
         int32_t fraction = 0;
         size_t fractionDigits = 0;
         while ((offsetIndex < text.size()) && (text[offsetIndex] >= '0') && (text[offsetIndex] <= '9'))
         {
            if (fractionDigits < 3)
            {
               fraction = fraction * 10 + (text[offsetIndex] - '0');
               fractionDigits += 1;
            }
            offsetIndex += 1;
         }
         while (fractionDigits < 3)
         {
            fraction *= 10;
            fractionDigits += 1;
         }
         dateTime.millisecond = fraction;
      }

      int32_t offset = 0;
      if (offsetIndex >= text.size())
      {
         return ParseResult::InvalidFormat;
      }
      if ((text[offsetIndex] == 'Z') || (text[offsetIndex] == 'z'))
      {
         offset = 0;
      }
      else if ((text[offsetIndex] == '+') || (text[offsetIndex] == '-'))
      {
         const int32_t sign = (text[offsetIndex] == '+') ? 1 : -1;
         offsetIndex += 1;
         int32_t offsetHours = 0;
         int32_t offsetMinutes = 0;
         if (ParseDigits(text, offsetIndex, 2, offsetHours) == 0)
         {
            return ParseResult::InvalidFormat;
         }
         offsetIndex += 2;
         if ((offsetIndex < text.size()) && (text[offsetIndex] == ':'))
         {
            offsetIndex += 1;
         }
         if (ParseDigits(text, offsetIndex, 2, offsetMinutes) == 0)
         {
            return ParseResult::InvalidFormat;
         }
         offset = sign * (offsetHours * static_cast<int32_t>(MinutesPerHour) + offsetMinutes);
      }
      else
      {
         return ParseResult::InvalidFormat;
      }

      utcOffsetMinutes = offset;
      unixTimeMs = CivilToUnixMs(dateTime, offset);
      return ParseResult::Ok;
   }
} // namespace LocationHistory
