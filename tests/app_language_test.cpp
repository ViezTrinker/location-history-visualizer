/*!
 *\file app_language_test.cpp
 *\brief Unit tests for UI language code mapping
 */

#include "app_language.h"

#include <gtest/gtest.h>

TEST(AppLanguage, LanguageCodeMatchesEnum)
{
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::English), "en");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::German), "de");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Spanish), "es");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::French), "fr");
}

TEST(AppLanguage, LanguageFromCodeFallsBackToEnglish)
{
   EXPECT_EQ(LocationHistory::LanguageFromCode("de"), LocationHistory::AppLanguage::German);
   EXPECT_EQ(LocationHistory::LanguageFromCode("es"), LocationHistory::AppLanguage::Spanish);
   EXPECT_EQ(LocationHistory::LanguageFromCode("fr"), LocationHistory::AppLanguage::French);
   EXPECT_EQ(LocationHistory::LanguageFromCode("en"), LocationHistory::AppLanguage::English);
   EXPECT_EQ(LocationHistory::LanguageFromCode(""), LocationHistory::AppLanguage::English);
   EXPECT_EQ(LocationHistory::LanguageFromCode("xx"), LocationHistory::AppLanguage::English);
}

TEST(AppLanguage, RoundTripKnownCodes)
{
   for (uint8_t languageValue = 0; languageValue < LocationHistory::AppLanguageCount; ++languageValue)
   {
      const auto language = static_cast<LocationHistory::AppLanguage>(languageValue);
      const LocationHistory::AppLanguage restored =
         LocationHistory::LanguageFromCode(LocationHistory::LanguageCode(language));
      EXPECT_EQ(restored, language);
   }
}
