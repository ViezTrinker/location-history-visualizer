/*!
 *\file clusterer.h
 *\brief Zoom-dependent grid clustering of location points
 */

#ifndef CLUSTERER_H
#define CLUSTERER_H

#include <cstdint>
#include <vector>

#include "location_data.h"

namespace LocationHistory
{
   inline constexpr int32_t ClusterCellSizePx = 48;

   struct Cluster
   {
      double latitude;
      double longitude;
      double minLatitude;
      double maxLatitude;
      double minLongitude;
      double maxLongitude;
      int32_t count;
   };

   using ClusterList = std::vector<Cluster>;

   /*!
    *\brief Groups nearby points into clusters for the given zoom level
    *
    *\param[in] points Location points to cluster
    *\param[in] zoom OSM zoom level
    *\param[in] cellSizePx Grid cell size in screen/world pixels
    *\param[out] clusters Resulting clusters
    */
   void BuildClusters(const LocationPointList& points, int32_t zoom, int32_t cellSizePx, ClusterList& clusters);
} // namespace LocationHistory

#endif // CLUSTERER_H
