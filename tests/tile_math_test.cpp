/*!
 *\file tile_math_test.cpp
 *\brief Unit tests for Web Mercator tile math
 */

#include "tile_math.h"

#include <cmath>

#include <gtest/gtest.h>

TEST(TileMath, PrimeMeridianEquatorAtMinZoom)
{
   const int32_t zoom = LocationHistory::MinZoom;
   const double mapSize = LocationHistory::MapSizePx(zoom);
   EXPECT_NEAR(LocationHistory::LongitudeToWorldX(0.0, zoom), mapSize / 2.0, 1.0e-6);
   EXPECT_NEAR(LocationHistory::LatitudeToWorldY(0.0, zoom), mapSize / 2.0, 1.0e-6);
}

TEST(TileMath, InverseLongitudeRoundTrip)
{
   const int32_t zoom = 10;
   const double longitude = 9.45;
   const double worldX = LocationHistory::LongitudeToWorldX(longitude, zoom);
   EXPECT_NEAR(LocationHistory::WorldXToLongitude(worldX, zoom), longitude, 1.0e-9);
}

TEST(TileMath, InverseLatitudeRoundTrip)
{
   const int32_t zoom = 10;
   const double latitude = 51.29;
   const double worldY = LocationHistory::LatitudeToWorldY(latitude, zoom);
   EXPECT_NEAR(LocationHistory::WorldYToLatitude(worldY, zoom), latitude, 1.0e-9);
}

TEST(TileMath, TileIndicesAreInRange)
{
   const int32_t zoom = 5;
   const int32_t tileX = LocationHistory::TileXFromLongitude(9.45, zoom);
   const int32_t tileY = LocationHistory::TileYFromLatitude(51.29, zoom);
   const int32_t tileCount = 1 << zoom;
   EXPECT_GE(tileX, 0);
   EXPECT_LT(tileX, tileCount);
   EXPECT_GE(tileY, 0);
   EXPECT_LT(tileY, tileCount);
}

TEST(TileMath, ClampZoomLimits)
{
   EXPECT_EQ(LocationHistory::ClampZoom(0), LocationHistory::MinZoom);
   EXPECT_EQ(LocationHistory::ClampZoom(99), LocationHistory::MaxZoom);
   EXPECT_EQ(LocationHistory::ClampZoom(8), 8);
}

TEST(TileMath, MapSizeGrowsWithZoom)
{
   EXPECT_NEAR(LocationHistory::MapSizePx(3), LocationHistory::MapSizePx(2) * 2.0, 1.0e-9);
}
