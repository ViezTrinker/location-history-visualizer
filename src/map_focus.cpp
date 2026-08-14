/*!
 *\file map_focus.cpp
 *\brief Finds a map center and zoom around the densest location cluster
 */

#include "map_focus.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>

#include "location_data.h"
#include "tile_math.h"

namespace LocationHistory
{
   namespace
   {
      using CellKey = uint64_t;

      CellKey MakeCellKey(const int32_t cellX, const int32_t cellY)
      {
         const auto packedX = static_cast<uint32_t>(cellX);
         const auto packedY = static_cast<uint32_t>(cellY);
         return (static_cast<uint64_t>(packedX) << 32) | static_cast<uint64_t>(packedY);
      }

      struct CellAccumulator
      {
         double latitudeSum = 0.0;
         double longitudeSum = 0.0;
         double minLatitude = 0.0;
         double maxLatitude = 0.0;
         double minLongitude = 0.0;
         double maxLongitude = 0.0;
         int32_t count = 0;
      };

      void AddToCell(CellAccumulator& accumulator, const LocationPoint& point)
      {
         if (accumulator.count == 0)
         {
            accumulator.minLatitude = point.latitude;
            accumulator.maxLatitude = point.latitude;
            accumulator.minLongitude = point.longitude;
            accumulator.maxLongitude = point.longitude;
         }
         else
         {
            accumulator.minLatitude = std::min(accumulator.minLatitude, point.latitude);
            accumulator.maxLatitude = std::max(accumulator.maxLatitude, point.latitude);
            accumulator.minLongitude = std::min(accumulator.minLongitude, point.longitude);
            accumulator.maxLongitude = std::max(accumulator.maxLongitude, point.longitude);
         }

         accumulator.latitudeSum += point.latitude;
         accumulator.longitudeSum += point.longitude;
         accumulator.count += 1;
      }

      int32_t ZoomThatFits(const CellAccumulator& accumulator, const int32_t viewWidthPx, const int32_t viewHeightPx)
      {
         const double latitudeSpan = std::max(
            (accumulator.maxLatitude - accumulator.minLatitude) * FocusPaddingFactor,
            MinFocusSpanDegrees);
         const double longitudeSpan = std::max(
            (accumulator.maxLongitude - accumulator.minLongitude) * FocusPaddingFactor,
            MinFocusSpanDegrees);

         const double centerLatitude = accumulator.latitudeSum / static_cast<double>(accumulator.count);
         const double centerLongitude = accumulator.longitudeSum / static_cast<double>(accumulator.count);

         int32_t chosenZoom = MinZoom;
         for (int32_t zoom = MaxZoom; zoom >= MinZoom; --zoom)
         {
            const double minX = LongitudeToWorldX(centerLongitude - longitudeSpan / 2.0, zoom);
            const double maxX = LongitudeToWorldX(centerLongitude + longitudeSpan / 2.0, zoom);
            const double minY = LatitudeToWorldY(centerLatitude + latitudeSpan / 2.0, zoom);
            const double maxY = LatitudeToWorldY(centerLatitude - latitudeSpan / 2.0, zoom);
            const double neededWidth = std::abs(maxX - minX);
            const double neededHeight = std::abs(maxY - minY);
            if ((neededWidth <= static_cast<double>(viewWidthPx)) &&
                (neededHeight <= static_cast<double>(viewHeightPx)))
            {
               chosenZoom = zoom;
               break;
            }
         }

         return chosenZoom;
      }
   } // namespace

   FocusResult ComputeDensestFocus(const LocationPointList& points, const int32_t viewWidthPx, const int32_t viewHeightPx, MapFocus& focus)
   {
      if (points.empty())
      {
         return FocusResult::NoPoints;
      }

      int32_t widthPx = viewWidthPx;
      int32_t heightPx = viewHeightPx;
      if (widthPx <= 0)
      {
         widthPx = DefaultFocusViewWidthPx;
      }
      if (heightPx <= 0)
      {
         heightPx = DefaultFocusViewHeightPx;
      }

      std::unordered_map<CellKey, CellAccumulator> cells;
      for (size_t index = 0; index < points.size(); ++index)
      {
         const LocationPoint& point = points[index];
         const auto cellX = static_cast<int32_t>(std::floor(point.longitude / DensityCellDegrees));
         const auto cellY = static_cast<int32_t>(std::floor(point.latitude / DensityCellDegrees));
         AddToCell(cells[MakeCellKey(cellX, cellY)], point);
      }

      const CellAccumulator* pBestCell = nullptr;
      for (const auto& entry : cells)
      {
         if ((pBestCell == nullptr) || (entry.second.count > pBestCell->count))
         {
            pBestCell = &entry.second;
         }
      }
      if (pBestCell == nullptr)
      {
         return FocusResult::NoPoints;
      }

      focus.latitude = pBestCell->latitudeSum / static_cast<double>(pBestCell->count);
      focus.longitude = pBestCell->longitudeSum / static_cast<double>(pBestCell->count);
      focus.zoom = ZoomThatFits(*pBestCell, widthPx, heightPx);
      return FocusResult::Ok;
   }
} // namespace LocationHistory
