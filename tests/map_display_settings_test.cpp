/*!
 *\file map_display_settings_test.cpp
 *\brief Unit tests for map overlay display limits
 */

#include "map_display_settings.h"

#include <gtest/gtest.h>

TEST(MapDisplaySettings, DefaultsMatchPreviousHardLimits)
{
   EXPECT_EQ(LocationHistory::DefaultDrawnPointLimit, 20000);
   EXPECT_EQ(LocationHistory::DefaultPointRadiusPx, 4);
}

TEST(MapDisplaySettings, ClampDrawnPointLimit)
{
   EXPECT_EQ(LocationHistory::ClampDrawnPointLimit(0), LocationHistory::MinDrawnPointLimit);
   EXPECT_EQ(
      LocationHistory::ClampDrawnPointLimit(LocationHistory::MinDrawnPointLimit - 1),
      LocationHistory::MinDrawnPointLimit);
   EXPECT_EQ(
      LocationHistory::ClampDrawnPointLimit(LocationHistory::DefaultDrawnPointLimit),
      LocationHistory::DefaultDrawnPointLimit);
   EXPECT_EQ(
      LocationHistory::ClampDrawnPointLimit(LocationHistory::MaxDrawnPointLimit + 1),
      LocationHistory::MaxDrawnPointLimit);
}

TEST(MapDisplaySettings, ClampPointRadiusPx)
{
   EXPECT_EQ(LocationHistory::ClampPointRadiusPx(0), LocationHistory::MinPointRadiusPx);
   EXPECT_EQ(
      LocationHistory::ClampPointRadiusPx(LocationHistory::DefaultPointRadiusPx),
      LocationHistory::DefaultPointRadiusPx);
   EXPECT_EQ(
      LocationHistory::ClampPointRadiusPx(LocationHistory::MaxPointRadiusPx + 8),
      LocationHistory::MaxPointRadiusPx);
}

TEST(MapDisplaySettings, DrawnPointStepIsOneWhenAtOrBelowLimit)
{
   EXPECT_EQ(LocationHistory::DrawnPointStep(100, LocationHistory::DefaultDrawnPointLimit), 1);
   EXPECT_EQ(
      LocationHistory::DrawnPointStep(
         static_cast<size_t>(LocationHistory::DefaultDrawnPointLimit),
         LocationHistory::DefaultDrawnPointLimit),
      1);
}

TEST(MapDisplaySettings, DrawnPointStepSkipsWhenOverLimit)
{
   EXPECT_EQ(LocationHistory::DrawnPointStep(40000, LocationHistory::DefaultDrawnPointLimit), 2);
   EXPECT_EQ(LocationHistory::DrawnPointStep(200000, LocationHistory::DefaultDrawnPointLimit), 10);
}

TEST(MapDisplaySettings, DrawnPointCountMatchesStrideLoop)
{
   EXPECT_EQ(
      LocationHistory::DrawnPointCount(40000, LocationHistory::DefaultDrawnPointLimit),
      static_cast<size_t>(20000));
   EXPECT_EQ(
      LocationHistory::DrawnPointCount(100, LocationHistory::DefaultDrawnPointLimit),
      static_cast<size_t>(100));
}
