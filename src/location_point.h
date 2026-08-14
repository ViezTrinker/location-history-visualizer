/*!
 *\file location_point.h
 *\brief Single location sample from Google Timeline data
 */

#ifndef LOCATION_POINT_H
#define LOCATION_POINT_H

#include <cstdint>

namespace LocationHistory
{
   enum class PointSource : uint8_t
   {
      TimelinePath = 0,
      Visit = 1,
      RawPosition = 2
   };

   struct LocationPoint
   {
      double latitude;
      double longitude;
      int64_t unixTimeMs;
      int32_t utcOffsetMinutes;
      PointSource source;
   };
} // namespace LocationHistory

#endif // LOCATION_POINT_H
