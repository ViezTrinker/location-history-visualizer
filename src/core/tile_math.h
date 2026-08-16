/*!
 *\file tile_math.h
 *\brief Web Mercator conversion between geographic coordinates and OSM tiles
 */

#ifndef TILE_MATH_H
#define TILE_MATH_H

#include <cstdint>

namespace LocationHistory
{
   inline constexpr int32_t TileSizePx = 256;
   inline constexpr int32_t MinZoom = 2;
   inline constexpr int32_t MaxZoom = 19;
   inline constexpr double MercatorMaxLatitude = 85.05112878;

   struct TileId
   {
      int32_t zoom;
      int32_t tileX;
      int32_t tileY;
   };

   inline bool operator==(const TileId& left, const TileId& right)
   {
      return (left.zoom == right.zoom) && (left.tileX == right.tileX) && (left.tileY == right.tileY);
   }

   /*!
    *\brief Converts longitude in degrees to world X in pixels at the given zoom
    *
    *\param[in] longitude Longitude in degrees
    *\param[in] zoom OSM zoom level
    */
   double LongitudeToWorldX(double longitude, int32_t zoom);

   /*!
    *\brief Converts latitude in degrees to world Y in pixels at the given zoom
    *
    *\param[in] latitude Latitude in degrees
    *\param[in] zoom OSM zoom level
    */
   double LatitudeToWorldY(double latitude, int32_t zoom);

   /*!
    *\brief Converts world X in pixels to longitude in degrees
    *
    *\param[in] worldX World X in pixels
    *\param[in] zoom OSM zoom level
    */
   double WorldXToLongitude(double worldX, int32_t zoom);

   /*!
    *\brief Converts world Y in pixels to latitude in degrees
    *
    *\param[in] worldY World Y in pixels
    *\param[in] zoom OSM zoom level
    */
   double WorldYToLatitude(double worldY, int32_t zoom);

   /*!
    *\brief Returns the OSM tile X index for a longitude
    *
    *\param[in] longitude Longitude in degrees
    *\param[in] zoom OSM zoom level
    */
   int32_t TileXFromLongitude(double longitude, int32_t zoom);

   /*!
    *\brief Returns the OSM tile Y index for a latitude
    *
    *\param[in] latitude Latitude in degrees
    *\param[in] zoom OSM zoom level
    */
   int32_t TileYFromLatitude(double latitude, int32_t zoom);

   /*!
    *\brief Returns the map width/height in pixels at the given zoom
    *
    *\param[in] zoom OSM zoom level
    */
   double MapSizePx(int32_t zoom);

   /*!
    *\brief Clamps a zoom level into the supported range
    *
    *\param[in] zoom Requested zoom level
    */
   int32_t ClampZoom(int32_t zoom);
} // namespace LocationHistory

#endif // TILE_MATH_H
