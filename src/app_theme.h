/*!
 *\file app_theme.h
 *\brief UI color theme selection
 */

#ifndef APP_THEME_H
#define APP_THEME_H

#include <cstdint>
#include <string_view>

namespace LocationHistory
{
   enum class AppTheme : uint8_t
   {
      Dark = 0,
      Light = 1,
      Midnight = 2,
      Nord = 3,
      Sepia = 4
   };

   inline constexpr size_t AppThemeCount = 5;
   inline constexpr AppTheme DefaultAppTheme = AppTheme::Dark;

   /*!
    *\brief Returns the settings key for a theme
    *
    *\param[in] theme Selected UI theme
    */
   inline std::string_view ThemeCode(const AppTheme theme)
   {
      if (theme == AppTheme::Light)
      {
         return "light";
      }
      if (theme == AppTheme::Midnight)
      {
         return "midnight";
      }
      if (theme == AppTheme::Nord)
      {
         return "nord";
      }
      if (theme == AppTheme::Sepia)
      {
         return "sepia";
      }
      return "dark";
   }

   /*!
    *\brief Maps a settings key to an AppTheme value
    *
    * Unknown codes fall back to Dark.
    *
    *\param[in] code Theme code such as dark, light, midnight, nord, or sepia
    */
   inline AppTheme ThemeFromCode(const std::string_view code)
   {
      if (code == "light")
      {
         return AppTheme::Light;
      }
      if (code == "midnight")
      {
         return AppTheme::Midnight;
      }
      if (code == "nord")
      {
         return AppTheme::Nord;
      }
      if (code == "sepia")
      {
         return AppTheme::Sepia;
      }
      return AppTheme::Dark;
   }

   /*!
    *\brief Reads the saved UI theme from QSettings
    */
   AppTheme LoadThemeSetting(void);

   /*!
    *\brief Stores the UI theme in QSettings
    *
    *\param[in] theme Theme to persist
    */
   void SaveThemeSetting(AppTheme theme);

   /*!
    *\brief Applies Fusion style and the matching color palette
    *
    * Dark is the default look. Light, Midnight, Nord, and Sepia are extra palettes.
    *
    *\param[in] theme Theme to apply
    */
   void ApplyAppTheme(AppTheme theme);

   /*!
    *\brief Applies the theme stored in QSettings
    */
   void ApplySavedAppTheme(void);
} // namespace LocationHistory

#endif // APP_THEME_H
