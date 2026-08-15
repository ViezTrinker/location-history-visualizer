/*!
 *\file map_display_settings.h
 *\brief Drawn-point limit and point radius for the map overlay
 */

#ifndef MAP_DISPLAY_SETTINGS_H
#define MAP_DISPLAY_SETTINGS_H

#include <cstddef>
#include <cstdint>

namespace LocationHistory
{
   inline constexpr int32_t DefaultDrawnPointLimit = 20000;
   inline constexpr int32_t MinDrawnPointLimit = 1000;
   inline constexpr int32_t MaxDrawnPointLimit = 1000000;
   inline constexpr int32_t DrawnPointLimitSpinStep = 1000;
   inline constexpr int32_t DefaultPointRadiusPx = 4;
   inline constexpr int32_t MinPointRadiusPx = 1;
   inline constexpr int32_t MaxPointRadiusPx = 16;

   /*!
    *\brief Clamps a drawn-point limit to the allowed range
    *
    *\param[in] drawnPointLimit Requested maximum number of drawn points
    */
   inline int32_t ClampDrawnPointLimit(const int32_t drawnPointLimit)
   {
      if (drawnPointLimit < MinDrawnPointLimit)
      {
         return MinDrawnPointLimit;
      }
      if (drawnPointLimit > MaxDrawnPointLimit)
      {
         return MaxDrawnPointLimit;
      }
      return drawnPointLimit;
   }

   /*!
    *\brief Clamps a point radius in pixels to the allowed range
    *
    *\param[in] pointRadiusPx Requested circle radius
    */
   inline int32_t ClampPointRadiusPx(const int32_t pointRadiusPx)
   {
      if (pointRadiusPx < MinPointRadiusPx)
      {
         return MinPointRadiusPx;
      }
      if (pointRadiusPx > MaxPointRadiusPx)
      {
         return MaxPointRadiusPx;
      }
      return pointRadiusPx;
   }

   /*!
    *\brief Returns the index stride used when too many points are drawn
    *
    *\param[in] pointCount Number of points in the overlay list
    *\param[in] drawnPointLimit Maximum number of circles to paint
    */
   inline int32_t DrawnPointStep(const size_t pointCount, const int32_t drawnPointLimit)
   {
      if (drawnPointLimit < 1)
      {
         return 1;
      }
      if (pointCount <= static_cast<size_t>(drawnPointLimit))
      {
         return 1;
      }

      const auto step = static_cast<int32_t>(pointCount / static_cast<size_t>(drawnPointLimit));
      if (step < 1)
      {
         return 1;
      }
      return step;
   }

   /*!
    *\brief Returns how many points the overlay actually paints
    *
    *\param[in] pointCount Number of points in the overlay list
    *\param[in] drawnPointLimit Maximum number of circles to paint
    */
   inline size_t DrawnPointCount(const size_t pointCount, const int32_t drawnPointLimit)
   {
      const int32_t step = DrawnPointStep(pointCount, drawnPointLimit);
      if (step <= 1)
      {
         return pointCount;
      }
      return (pointCount + static_cast<size_t>(step) - 1) / static_cast<size_t>(step);
   }

   /*!
    *\brief Reads the saved drawn-point limit from QSettings
    */
   int32_t LoadDrawnPointLimit(void);

   /*!
    *\brief Stores the drawn-point limit in QSettings
    *
    *\param[in] drawnPointLimit Maximum number of drawn points
    */
   void SaveDrawnPointLimit(int32_t drawnPointLimit);

   /*!
    *\brief Reads the saved point radius from QSettings
    */
   int32_t LoadPointRadiusPx(void);

   /*!
    *\brief Stores the point radius in QSettings
    *
    *\param[in] pointRadiusPx Circle radius in pixels
    */
   void SavePointRadiusPx(int32_t pointRadiusPx);
} // namespace LocationHistory

#endif // MAP_DISPLAY_SETTINGS_H
