/*!
 *\file app_language.h
 *\brief UI language selection and Qt translator loading
 */

#ifndef APP_LANGUAGE_H
#define APP_LANGUAGE_H

#include <cstdint>
#include <string_view>

namespace LocationHistory
{
   enum class AppLanguage : uint8_t
   {
      English = 0,
      German = 1,
      Spanish = 2,
      French = 3,
      Russian = 4,
      Arabic = 5,
      Italian = 6,
      Turkish = 7,
      Dutch = 8,
      Portuguese = 9,
      Polish = 10
   };

   enum class TextDirection : uint8_t
   {
      LeftToRight = 0,
      RightToLeft = 1
   };

   inline constexpr size_t AppLanguageCount = 11;
   inline constexpr AppLanguage DefaultAppLanguage = AppLanguage::English;

   /*!
    *\brief Returns the ISO language code used in settings and .qm names
    *
    *\param[in] language Selected UI language
    */
   inline std::string_view LanguageCode(const AppLanguage language)
   {
      if (language == AppLanguage::German)
      {
         return "de";
      }
      if (language == AppLanguage::Spanish)
      {
         return "es";
      }
      if (language == AppLanguage::French)
      {
         return "fr";
      }
      if (language == AppLanguage::Russian)
      {
         return "ru";
      }
      if (language == AppLanguage::Arabic)
      {
         return "ar";
      }
      if (language == AppLanguage::Italian)
      {
         return "it";
      }
      if (language == AppLanguage::Turkish)
      {
         return "tr";
      }
      if (language == AppLanguage::Dutch)
      {
         return "nl";
      }
      if (language == AppLanguage::Portuguese)
      {
         return "pt";
      }
      if (language == AppLanguage::Polish)
      {
         return "pl";
      }
      return "en";
   }

   /*!
    *\brief Returns the language name in that language
    *
    *\param[in] language Selected UI language
    */
   inline std::string_view LanguageNativeName(const AppLanguage language)
   {
      if (language == AppLanguage::German)
      {
         return "Deutsch";
      }
      if (language == AppLanguage::Spanish)
      {
         return "Español";
      }
      if (language == AppLanguage::French)
      {
         return "Français";
      }
      if (language == AppLanguage::Russian)
      {
         return "Русский";
      }
      if (language == AppLanguage::Arabic)
      {
         return "العربية";
      }
      if (language == AppLanguage::Italian)
      {
         return "Italiano";
      }
      if (language == AppLanguage::Turkish)
      {
         return "Türkçe";
      }
      if (language == AppLanguage::Dutch)
      {
         return "Nederlands";
      }
      if (language == AppLanguage::Portuguese)
      {
         return "Português";
      }
      if (language == AppLanguage::Polish)
      {
         return "Polski";
      }
      return "English";
   }

   /*!
    *\brief Returns the text direction for a UI language
    *
    *\param[in] language Selected UI language
    */
   inline TextDirection LanguageTextDirection(const AppLanguage language)
   {
      if (language == AppLanguage::Arabic)
      {
         return TextDirection::RightToLeft;
      }
      return TextDirection::LeftToRight;
   }

   /*!
    *\brief Maps an ISO language code to an AppLanguage value
    *
    * Unknown codes fall back to English.
    *
    *\param[in] code Language code such as en, de, es, fr, ru, ar, it, tr, nl, pt, pl
    */
   inline AppLanguage LanguageFromCode(const std::string_view code)
   {
      if (code == "de")
      {
         return AppLanguage::German;
      }
      if (code == "es")
      {
         return AppLanguage::Spanish;
      }
      if (code == "fr")
      {
         return AppLanguage::French;
      }
      if (code == "ru")
      {
         return AppLanguage::Russian;
      }
      if (code == "ar")
      {
         return AppLanguage::Arabic;
      }
      if (code == "it")
      {
         return AppLanguage::Italian;
      }
      if (code == "tr")
      {
         return AppLanguage::Turkish;
      }
      if (code == "nl")
      {
         return AppLanguage::Dutch;
      }
      if (code == "pt")
      {
         return AppLanguage::Portuguese;
      }
      if (code == "pl")
      {
         return AppLanguage::Polish;
      }
      return AppLanguage::English;
   }

   /*!
    *\brief Reads the saved UI language from QSettings
    */
   AppLanguage LoadLanguageSetting(void);

   /*!
    *\brief Stores the UI language in QSettings
    *
    *\param[in] language Language to persist
    */
   void SaveLanguageSetting(AppLanguage language);

   /*!
    *\brief Installs translators and the matching QLocale
    *
    * English removes extra translators so source strings are used.
    *
    *\param[in] language Language to apply
    */
   void ApplyAppLanguage(AppLanguage language);

   /*!
    *\brief Applies the language stored in QSettings
    */
   void ApplySavedAppLanguage(void);
} // namespace LocationHistory

#endif // APP_LANGUAGE_H
