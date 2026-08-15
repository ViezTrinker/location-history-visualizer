/*!
 *\file app_theme.cpp
 *\brief UI color theme selection
 */

#include "app_theme.h"

#include <string_view>

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QSettings>
#include <QString>
#include <QStyle>
#include <QStyleFactory>

namespace LocationHistory
{
   namespace
   {
      QString ThemeSettingsKey(void)
      {
         return QStringLiteral("theme");
      }

      QString ThemeCodeString(const AppTheme theme)
      {
         const std::string_view code = ThemeCode(theme);
         return QString::fromUtf8(code.data(), static_cast<int>(code.size()));
      }

      QPalette MakeDarkPalette(void)
      {
         QPalette palette;
         palette.setColor(QPalette::Window, QColor(45, 45, 45));
         palette.setColor(QPalette::WindowText, QColor(235, 235, 235));
         palette.setColor(QPalette::Base, QColor(32, 32, 32));
         palette.setColor(QPalette::AlternateBase, QColor(54, 54, 54));
         palette.setColor(QPalette::ToolTipBase, QColor(45, 45, 45));
         palette.setColor(QPalette::ToolTipText, QColor(235, 235, 235));
         palette.setColor(QPalette::Text, QColor(235, 235, 235));
         palette.setColor(QPalette::Button, QColor(58, 58, 58));
         palette.setColor(QPalette::ButtonText, QColor(235, 235, 235));
         palette.setColor(QPalette::BrightText, QColor(255, 90, 90));
         palette.setColor(QPalette::Link, QColor(90, 160, 230));
         palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
         palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
         palette.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));
         palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(130, 130, 130));
         palette.setColor(QPalette::Disabled, QPalette::Text, QColor(130, 130, 130));
         palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(130, 130, 130));
         palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
         palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(160, 160, 160));
         return palette;
      }

      QPalette MakeLightPalette(void)
      {
         QPalette palette;
         palette.setColor(QPalette::Window, QColor(245, 245, 245));
         palette.setColor(QPalette::WindowText, QColor(32, 32, 32));
         palette.setColor(QPalette::Base, QColor(255, 255, 255));
         palette.setColor(QPalette::AlternateBase, QColor(236, 236, 236));
         palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 240));
         palette.setColor(QPalette::ToolTipText, QColor(32, 32, 32));
         palette.setColor(QPalette::Text, QColor(32, 32, 32));
         palette.setColor(QPalette::Button, QColor(240, 240, 240));
         palette.setColor(QPalette::ButtonText, QColor(32, 32, 32));
         palette.setColor(QPalette::BrightText, QColor(180, 0, 0));
         palette.setColor(QPalette::Link, QColor(0, 102, 204));
         palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
         palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
         palette.setColor(QPalette::PlaceholderText, QColor(120, 120, 120));
         palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(140, 140, 140));
         palette.setColor(QPalette::Disabled, QPalette::Text, QColor(140, 140, 140));
         palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(140, 140, 140));
         palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(200, 200, 200));
         palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(80, 80, 80));
         return palette;
      }

      void ApplyFusionStyle(void)
      {
         QStyle* pStyle = QStyleFactory::create(QStringLiteral("Fusion"));
         if (pStyle == nullptr)
         {
            return;
         }
         QApplication::setStyle(pStyle);
      }
   } // namespace

   AppTheme LoadThemeSetting(void)
   {
      QSettings settings;
      const QString storedCode = settings.value(ThemeSettingsKey()).toString();
      if (storedCode.isEmpty())
      {
         return DefaultAppTheme;
      }

      return ThemeFromCode(storedCode.toStdString());
   }

   void SaveThemeSetting(const AppTheme theme)
   {
      QSettings settings;
      settings.setValue(ThemeSettingsKey(), ThemeCodeString(theme));
   }

   void ApplyAppTheme(const AppTheme theme)
   {
      ApplyFusionStyle();
      if (theme == AppTheme::Light)
      {
         QApplication::setPalette(MakeLightPalette());
         return;
      }
      QApplication::setPalette(MakeDarkPalette());
   }

   void ApplySavedAppTheme(void)
   {
      ApplyAppTheme(LoadThemeSetting());
   }
} // namespace LocationHistory
