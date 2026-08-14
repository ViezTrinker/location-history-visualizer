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
      French = 3
   };

   inline constexpr size_t AppLanguageCount = 4;
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
      return "English";
   }

   /*!
    *\brief Maps an ISO language code to an AppLanguage value
    *
    * Unknown codes fall back to English.
    *
    *\param[in] code Language code such as en, de, es, fr
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
