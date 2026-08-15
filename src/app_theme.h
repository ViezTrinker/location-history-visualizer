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
      Light = 1
   };

   inline constexpr size_t AppThemeCount = 2;
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
      return "dark";
   }

   /*!
    *\brief Maps a settings key to an AppTheme value
    *
    * Unknown codes fall back to Dark.
    *
    *\param[in] code Theme code such as dark or light
    */
   inline AppTheme ThemeFromCode(const std::string_view code)
   {
      if (code == "light")
      {
         return AppTheme::Light;
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
    * Dark is the default look. Light uses a pale Fusion palette.
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
