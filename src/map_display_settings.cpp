/*!
 *\file map_display_settings.cpp
 *\brief Drawn-point limit and point radius for the map overlay
 */

#include "map_display_settings.h"

#include <cstdint>

#include <QSettings>
#include <QString>

namespace LocationHistory
{
   namespace
   {
      QString DrawnPointLimitSettingsKey(void)
      {
         return QStringLiteral("drawnPointLimit");
      }

      QString PointRadiusPxSettingsKey(void)
      {
         return QStringLiteral("pointRadiusPx");
      }
   } // namespace

   int32_t LoadDrawnPointLimit(void)
   {
      QSettings settings;
      if (!settings.contains(DrawnPointLimitSettingsKey()))
      {
         return DefaultDrawnPointLimit;
      }

      const auto storedValue = static_cast<int32_t>(settings.value(DrawnPointLimitSettingsKey()).toInt());
      return ClampDrawnPointLimit(storedValue);
   }

   void SaveDrawnPointLimit(const int32_t drawnPointLimit)
   {
      QSettings settings;
      settings.setValue(DrawnPointLimitSettingsKey(), ClampDrawnPointLimit(drawnPointLimit));
   }

   int32_t LoadPointRadiusPx(void)
   {
      QSettings settings;
      if (!settings.contains(PointRadiusPxSettingsKey()))
      {
         return DefaultPointRadiusPx;
      }

      const auto storedValue = static_cast<int32_t>(settings.value(PointRadiusPxSettingsKey()).toInt());
      return ClampPointRadiusPx(storedValue);
   }

   void SavePointRadiusPx(const int32_t pointRadiusPx)
   {
      QSettings settings;
      settings.setValue(PointRadiusPxSettingsKey(), ClampPointRadiusPx(pointRadiusPx));
   }
} // namespace LocationHistory
