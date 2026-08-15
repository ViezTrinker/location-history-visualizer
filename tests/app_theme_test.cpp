/*!
 *\file app_theme_test.cpp
 *\brief Unit tests for UI theme code mapping
 */

#include "app_theme.h"

#include <gtest/gtest.h>

TEST(AppTheme, ThemeCodeMatchesEnum)
{
   EXPECT_EQ(LocationHistory::ThemeCode(LocationHistory::AppTheme::Dark), "dark");
   EXPECT_EQ(LocationHistory::ThemeCode(LocationHistory::AppTheme::Light), "light");
   EXPECT_EQ(LocationHistory::ThemeCode(LocationHistory::AppTheme::Midnight), "midnight");
   EXPECT_EQ(LocationHistory::ThemeCode(LocationHistory::AppTheme::Nord), "nord");
   EXPECT_EQ(LocationHistory::ThemeCode(LocationHistory::AppTheme::Sepia), "sepia");
}

TEST(AppTheme, ThemeFromCodeFallsBackToDark)
{
   EXPECT_EQ(LocationHistory::ThemeFromCode("light"), LocationHistory::AppTheme::Light);
   EXPECT_EQ(LocationHistory::ThemeFromCode("dark"), LocationHistory::AppTheme::Dark);
   EXPECT_EQ(LocationHistory::ThemeFromCode("midnight"), LocationHistory::AppTheme::Midnight);
   EXPECT_EQ(LocationHistory::ThemeFromCode("nord"), LocationHistory::AppTheme::Nord);
   EXPECT_EQ(LocationHistory::ThemeFromCode("sepia"), LocationHistory::AppTheme::Sepia);
   EXPECT_EQ(LocationHistory::ThemeFromCode(""), LocationHistory::AppTheme::Dark);
   EXPECT_EQ(LocationHistory::ThemeFromCode("xx"), LocationHistory::AppTheme::Dark);
}

TEST(AppTheme, DefaultIsDark)
{
   EXPECT_EQ(LocationHistory::DefaultAppTheme, LocationHistory::AppTheme::Dark);
}

TEST(AppTheme, RoundTripKnownCodes)
{
   for (uint8_t themeValue = 0; themeValue < LocationHistory::AppThemeCount; ++themeValue)
   {
      const auto theme = static_cast<LocationHistory::AppTheme>(themeValue);
      const LocationHistory::AppTheme restored =
         LocationHistory::ThemeFromCode(LocationHistory::ThemeCode(theme));
      EXPECT_EQ(restored, theme);
   }
}
