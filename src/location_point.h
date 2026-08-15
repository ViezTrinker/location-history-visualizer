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

   inline constexpr int32_t NoPathId = -1;

   struct LocationPoint
   {
      double latitude = 0.0;
      double longitude = 0.0;
      int64_t unixTimeMs = 0;
      int32_t utcOffsetMinutes = 0;
      PointSource source = PointSource::RawPosition;
      int64_t endUnixTimeMs = 0;
      int32_t pathId = NoPathId;
   };

   /*!
    *\brief Returns true if the sample has a stay duration after its start
    *
    *\param[in] point Location sample
    */
   inline bool PointHasDuration(const LocationPoint& point)
   {
      return point.endUnixTimeMs > point.unixTimeMs;
   }

   /*!
    *\brief Returns true if the sample has started at or before the cutoff
    *
    *\param[in] point Location sample
    *\param[in] untilUnixTimeMs Inclusive end of the visible story window
    */
   inline bool PointVisibleUntil(const LocationPoint& point, const int64_t untilUnixTimeMs)
   {
      return point.unixTimeMs <= untilUnixTimeMs;
   }
} // namespace LocationHistory

#endif // LOCATION_POINT_H
