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

      QPalette MakeMidnightPalette(void)
      {
         QPalette palette;
         palette.setColor(QPalette::Window, QColor(16, 16, 18));
         palette.setColor(QPalette::WindowText, QColor(240, 240, 245));
         palette.setColor(QPalette::Base, QColor(8, 8, 10));
         palette.setColor(QPalette::AlternateBase, QColor(28, 28, 32));
         palette.setColor(QPalette::ToolTipBase, QColor(16, 16, 18));
         palette.setColor(QPalette::ToolTipText, QColor(240, 240, 245));
         palette.setColor(QPalette::Text, QColor(240, 240, 245));
         palette.setColor(QPalette::Button, QColor(24, 24, 28));
         palette.setColor(QPalette::ButtonText, QColor(240, 240, 245));
         palette.setColor(QPalette::BrightText, QColor(255, 88, 88));
         palette.setColor(QPalette::Link, QColor(120, 175, 245));
         palette.setColor(QPalette::Highlight, QColor(56, 118, 220));
         palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
         palette.setColor(QPalette::PlaceholderText, QColor(110, 110, 120));
         palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(95, 95, 105));
         palette.setColor(QPalette::Disabled, QPalette::Text, QColor(95, 95, 105));
         palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(95, 95, 105));
         palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(40, 40, 48));
         palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(140, 140, 150));
         return palette;
      }

      QPalette MakeNordPalette(void)
      {
         QPalette palette;
         palette.setColor(QPalette::Window, QColor(46, 52, 64));
         palette.setColor(QPalette::WindowText, QColor(216, 222, 233));
         palette.setColor(QPalette::Base, QColor(36, 41, 51));
         palette.setColor(QPalette::AlternateBase, QColor(59, 66, 82));
         palette.setColor(QPalette::ToolTipBase, QColor(59, 66, 82));
         palette.setColor(QPalette::ToolTipText, QColor(236, 239, 244));
         palette.setColor(QPalette::Text, QColor(236, 239, 244));
         palette.setColor(QPalette::Button, QColor(59, 66, 82));
         palette.setColor(QPalette::ButtonText, QColor(216, 222, 233));
         palette.setColor(QPalette::BrightText, QColor(191, 97, 106));
         palette.setColor(QPalette::Link, QColor(136, 192, 208));
         palette.setColor(QPalette::Highlight, QColor(94, 129, 172));
         palette.setColor(QPalette::HighlightedText, QColor(236, 239, 244));
         palette.setColor(QPalette::PlaceholderText, QColor(76, 86, 106));
         palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(76, 86, 106));
         palette.setColor(QPalette::Disabled, QPalette::Text, QColor(76, 86, 106));
         palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(76, 86, 106));
         palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(67, 76, 94));
         palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(216, 222, 233));
         return palette;
      }

      QPalette MakeSepiaPalette(void)
      {
         QPalette palette;
         palette.setColor(QPalette::Window, QColor(232, 220, 196));
         palette.setColor(QPalette::WindowText, QColor(58, 42, 28));
         palette.setColor(QPalette::Base, QColor(247, 239, 220));
         palette.setColor(QPalette::AlternateBase, QColor(224, 208, 180));
         palette.setColor(QPalette::ToolTipBase, QColor(247, 239, 220));
         palette.setColor(QPalette::ToolTipText, QColor(58, 42, 28));
         palette.setColor(QPalette::Text, QColor(58, 42, 28));
         palette.setColor(QPalette::Button, QColor(226, 210, 182));
         palette.setColor(QPalette::ButtonText, QColor(58, 42, 28));
         palette.setColor(QPalette::BrightText, QColor(154, 48, 32));
         palette.setColor(QPalette::Link, QColor(122, 74, 36));
         palette.setColor(QPalette::Highlight, QColor(139, 90, 43));
         palette.setColor(QPalette::HighlightedText, QColor(255, 250, 240));
         palette.setColor(QPalette::PlaceholderText, QColor(138, 118, 90));
         palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(150, 132, 108));
         palette.setColor(QPalette::Disabled, QPalette::Text, QColor(150, 132, 108));
         palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(150, 132, 108));
         palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(200, 184, 156));
         palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(80, 64, 48));
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
      if (theme == AppTheme::Midnight)
      {
         QApplication::setPalette(MakeMidnightPalette());
         return;
      }
      if (theme == AppTheme::Nord)
      {
         QApplication::setPalette(MakeNordPalette());
         return;
      }
      if (theme == AppTheme::Sepia)
      {
         QApplication::setPalette(MakeSepiaPalette());
         return;
      }
      QApplication::setPalette(MakeDarkPalette());
   }

   void ApplySavedAppTheme(void)
   {
      ApplyAppTheme(LoadThemeSetting());
   }
} // namespace LocationHistory
