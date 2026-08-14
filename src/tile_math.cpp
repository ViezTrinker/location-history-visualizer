/*!
 *\file tile_math.cpp
 *\brief Web Mercator conversion between geographic coordinates and OSM tiles
 */

#include "tile_math.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace LocationHistory
{
   namespace
   {
      inline constexpr double Pi = 3.14159265358979323846;
      inline constexpr double DegreesToRadians = Pi / 180.0;
      inline constexpr double RadiansToDegrees = 180.0 / Pi;

      double ZoomMapSize(const int32_t zoom)
      {
         const int32_t clampedZoom = ClampZoom(zoom);
         const auto tileCount = static_cast<int64_t>(1) << clampedZoom;
         return static_cast<double>(TileSizePx) * static_cast<double>(tileCount);
      }
   } // namespace

   double MapSizePx(const int32_t zoom)
   {
      return ZoomMapSize(zoom);
   }

   int32_t ClampZoom(const int32_t zoom)
   {
      if (zoom < MinZoom)
      {
         return MinZoom;
      }
      if (zoom > MaxZoom)
      {
         return MaxZoom;
      }
      return zoom;
   }

   double LongitudeToWorldX(const double longitude, const int32_t zoom)
   {
      const double mapSize = ZoomMapSize(zoom);
      const double wrappedLongitude = std::clamp(longitude, -180.0, 180.0);
      return (wrappedLongitude + 180.0) / 360.0 * mapSize;
   }

   double LatitudeToWorldY(const double latitude, const int32_t zoom)
   {
      const double mapSize = ZoomMapSize(zoom);
      const double clampedLatitude = std::clamp(latitude, -MercatorMaxLatitude, MercatorMaxLatitude);
      const double latitudeRadians = clampedLatitude * DegreesToRadians;
      const double mercatorY = std::log(std::tan(Pi / 4.0 + latitudeRadians / 2.0));
      return (1.0 - mercatorY / Pi) / 2.0 * mapSize;
   }

   double WorldXToLongitude(const double worldX, const int32_t zoom)
   {
      const double mapSize = ZoomMapSize(zoom);
      return worldX / mapSize * 360.0 - 180.0;
   }

   double WorldYToLatitude(const double worldY, const int32_t zoom)
   {
      const double mapSize = ZoomMapSize(zoom);
      const double normalizedY = 1.0 - 2.0 * worldY / mapSize;
      const double latitudeRadians = std::atan(std::sinh(Pi * normalizedY));
      return latitudeRadians * RadiansToDegrees;
   }

   int32_t TileXFromLongitude(const double longitude, const int32_t zoom)
   {
      const double worldX = LongitudeToWorldX(longitude, zoom);
      const auto tileX = static_cast<int32_t>(std::floor(worldX / static_cast<double>(TileSizePx)));
      const int32_t clampedZoom = ClampZoom(zoom);
      const auto tileCount = static_cast<int32_t>(static_cast<int64_t>(1) << clampedZoom);
      if (tileX < 0)
      {
         return 0;
      }
      if (tileX >= tileCount)
      {
         return tileCount - 1;
      }
      return tileX;
   }

   int32_t TileYFromLatitude(const double latitude, const int32_t zoom)
   {
      const double worldY = LatitudeToWorldY(latitude, zoom);
      const auto tileY = static_cast<int32_t>(std::floor(worldY / static_cast<double>(TileSizePx)));
      const int32_t clampedZoom = ClampZoom(zoom);
      const auto tileCount = static_cast<int32_t>(static_cast<int64_t>(1) << clampedZoom);
      if (tileY < 0)
      {
         return 0;
      }
      if (tileY >= tileCount)
      {
         return tileCount - 1;
      }
      return tileY;
   }
} // namespace LocationHistory
