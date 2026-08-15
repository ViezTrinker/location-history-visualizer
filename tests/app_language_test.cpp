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
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Russian), "ru");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Arabic), "ar");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Italian), "it");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Turkish), "tr");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Dutch), "nl");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Portuguese), "pt");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Polish), "pl");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Japanese), "ja");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Korean), "ko");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Indonesian), "id");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Vietnamese), "vi");
   EXPECT_EQ(LocationHistory::LanguageCode(LocationHistory::AppLanguage::Hindi), "hi");
}

TEST(AppLanguage, LanguageFromCodeFallsBackToEnglish)
{
   EXPECT_EQ(LocationHistory::LanguageFromCode("de"), LocationHistory::AppLanguage::German);
   EXPECT_EQ(LocationHistory::LanguageFromCode("es"), LocationHistory::AppLanguage::Spanish);
   EXPECT_EQ(LocationHistory::LanguageFromCode("fr"), LocationHistory::AppLanguage::French);
   EXPECT_EQ(LocationHistory::LanguageFromCode("ru"), LocationHistory::AppLanguage::Russian);
   EXPECT_EQ(LocationHistory::LanguageFromCode("ar"), LocationHistory::AppLanguage::Arabic);
   EXPECT_EQ(LocationHistory::LanguageFromCode("it"), LocationHistory::AppLanguage::Italian);
   EXPECT_EQ(LocationHistory::LanguageFromCode("tr"), LocationHistory::AppLanguage::Turkish);
   EXPECT_EQ(LocationHistory::LanguageFromCode("nl"), LocationHistory::AppLanguage::Dutch);
   EXPECT_EQ(LocationHistory::LanguageFromCode("pt"), LocationHistory::AppLanguage::Portuguese);
   EXPECT_EQ(LocationHistory::LanguageFromCode("pl"), LocationHistory::AppLanguage::Polish);
   EXPECT_EQ(LocationHistory::LanguageFromCode("ja"), LocationHistory::AppLanguage::Japanese);
   EXPECT_EQ(LocationHistory::LanguageFromCode("ko"), LocationHistory::AppLanguage::Korean);
   EXPECT_EQ(LocationHistory::LanguageFromCode("id"), LocationHistory::AppLanguage::Indonesian);
   EXPECT_EQ(LocationHistory::LanguageFromCode("vi"), LocationHistory::AppLanguage::Vietnamese);
   EXPECT_EQ(LocationHistory::LanguageFromCode("hi"), LocationHistory::AppLanguage::Hindi);
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

TEST(AppLanguage, ArabicUsesRightToLeft)
{
   EXPECT_EQ(
      LocationHistory::LanguageTextDirection(LocationHistory::AppLanguage::Arabic),
      LocationHistory::TextDirection::RightToLeft);
   EXPECT_EQ(
      LocationHistory::LanguageTextDirection(LocationHistory::AppLanguage::Russian),
      LocationHistory::TextDirection::LeftToRight);
   EXPECT_EQ(
      LocationHistory::LanguageTextDirection(LocationHistory::AppLanguage::English),
      LocationHistory::TextDirection::LeftToRight);
   EXPECT_EQ(
      LocationHistory::LanguageTextDirection(LocationHistory::AppLanguage::Hindi),
      LocationHistory::TextDirection::LeftToRight);
   EXPECT_EQ(
      LocationHistory::LanguageTextDirection(LocationHistory::AppLanguage::Japanese),
      LocationHistory::TextDirection::LeftToRight);
}

TEST(AppLanguage, NativeNamesAreNonEmpty)
{
   for (uint8_t languageValue = 0; languageValue < LocationHistory::AppLanguageCount; ++languageValue)
   {
      const auto language = static_cast<LocationHistory::AppLanguage>(languageValue);
      EXPECT_FALSE(LocationHistory::LanguageNativeName(language).empty());
   }
}
