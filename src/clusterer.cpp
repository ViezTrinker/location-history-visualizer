/*!
 *\file clusterer.cpp
 *\brief Zoom-dependent grid clustering of location points
 */

#include "clusterer.h"

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <utility>

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
         int32_t count = 0;
      };
   } // namespace

   void BuildClusters(const LocationPointList& points, const int32_t zoom, const int32_t cellSizePx, ClusterList& clusters)
   {
      clusters.clear();
      if (points.empty())
      {
         return;
      }

      int32_t gridSize = cellSizePx;
      if (gridSize <= 0)
      {
         gridSize = ClusterCellSizePx;
      }

      std::unordered_map<CellKey, CellAccumulator> cells;
      for (size_t index = 0; index < points.size(); ++index)
      {
         const LocationPoint& point = points[index];
         const double worldX = LongitudeToWorldX(point.longitude, zoom);
         const double worldY = LatitudeToWorldY(point.latitude, zoom);
         const auto cellX = static_cast<int32_t>(std::floor(worldX / static_cast<double>(gridSize)));
         const auto cellY = static_cast<int32_t>(std::floor(worldY / static_cast<double>(gridSize)));
         const CellKey key = MakeCellKey(cellX, cellY);

         CellAccumulator& accumulator = cells[key];
         accumulator.latitudeSum += point.latitude;
         accumulator.longitudeSum += point.longitude;
         accumulator.count += 1;
      }

      clusters.reserve(cells.size());
      for (const auto& entry : cells)
      {
         const CellAccumulator& accumulator = entry.second;
         if (accumulator.count <= 0)
         {
            continue;
         }

         Cluster cluster{};
         cluster.latitude = accumulator.latitudeSum / static_cast<double>(accumulator.count);
         cluster.longitude = accumulator.longitudeSum / static_cast<double>(accumulator.count);
         cluster.count = accumulator.count;
         clusters.push_back(cluster);
      }
   }
} // namespace LocationHistory
