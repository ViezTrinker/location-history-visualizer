/*!
 *\file map_focus.h
 *\brief Finds a map center and zoom around the densest location cluster
 */

#ifndef MAP_FOCUS_H
#define MAP_FOCUS_H

#include <cstdint>

#include "location_data.h"

namespace LocationHistory
{
   inline constexpr double DensityCellDegrees = 0.02;
   inline constexpr double FocusPaddingFactor = 1.5;
   inline constexpr double MinFocusSpanDegrees = 0.003;
   inline constexpr int32_t DefaultFocusViewWidthPx = 800;
   inline constexpr int32_t DefaultFocusViewHeightPx = 600;

   enum class FocusResult : int8_t
   {
      NoPoints = -1,
      Ok = 0
   };

   struct MapFocus
   {
      double latitude;
      double longitude;
      int32_t zoom;
   };

   /*!
    *\brief Returns true if focus computation failed
    *
    *\param[in] result Focus result to check
    */
   inline bool IsErr(const FocusResult result)
   {
      return result < FocusResult::Ok;
   }

   /*!
    *\brief Returns true if focus computation succeeded
    *
    *\param[in] result Focus result to check
    */
   inline bool IsOk(const FocusResult result)
   {
      return result == FocusResult::Ok;
   }

   /*!
    *\brief Computes center and zoom for the cell that contains the most points
    *
    *\param[in] points Location points
    *\param[in] viewWidthPx Map widget width in pixels
    *\param[in] viewHeightPx Map widget height in pixels
    *\param[out] focus Center coordinates and zoom
    */
   FocusResult ComputeDensestFocus(const LocationPointList& points, int32_t viewWidthPx, int32_t viewHeightPx, MapFocus& focus);

   /*!
    *\brief Computes a zoom that fits a geographic span into the viewport
    *
    *\param[in] latitude Center latitude
    *\param[in] longitude Center longitude
    *\param[in] minLatitude Southern bound
    *\param[in] maxLatitude Northern bound
    *\param[in] minLongitude Western bound
    *\param[in] maxLongitude Eastern bound
    *\param[in] viewWidthPx Map widget width in pixels
    *\param[in] viewHeightPx Map widget height in pixels
    *\param[out] focus Center coordinates and zoom
    */
   FocusResult ComputeSpanFocus(
      double latitude,
      double longitude,
      double minLatitude,
      double maxLatitude,
      double minLongitude,
      double maxLongitude,
      int32_t viewWidthPx,
      int32_t viewHeightPx,
      MapFocus& focus);
} // namespace LocationHistory

#endif // MAP_FOCUS_H
