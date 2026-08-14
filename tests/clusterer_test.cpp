/*!
 *\file clusterer_test.cpp
 *\brief Unit tests for grid clustering
 */

#include "clusterer.h"

#include <gtest/gtest.h>

#include "location_data.h"
#include "location_point.h"
#include "tile_math.h"

TEST(Clusterer, EmptyInputYieldsNoClusters)
{
   LocationHistory::LocationPointList points;
   LocationHistory::ClusterList clusters;
   LocationHistory::BuildClusters(points, 12, LocationHistory::ClusterCellSizePx, clusters);
   EXPECT_TRUE(clusters.empty());
}

TEST(Clusterer, IdenticalPointsBecomeOneCluster)
{
   LocationHistory::LocationPointList points(3);
   for (size_t index = 0; index < points.size(); ++index)
   {
      points[index].latitude = 50.11;
      points[index].longitude = 8.68;
      points[index].unixTimeMs = 0;
      points[index].utcOffsetMinutes = 0;
      points[index].source = LocationHistory::PointSource::TimelinePath;
   }

   LocationHistory::ClusterList clusters;
   LocationHistory::BuildClusters(points, 14, LocationHistory::ClusterCellSizePx, clusters);
   ASSERT_EQ(clusters.size(), 1u);
   EXPECT_EQ(clusters[0].count, 3);
   EXPECT_NEAR(clusters[0].latitude, 50.11, 1.0e-9);
   EXPECT_NEAR(clusters[0].longitude, 8.68, 1.0e-9);
}

TEST(Clusterer, DistantPointsStaySeparateAtHighZoom)
{
   LocationHistory::LocationPointList points(2);
   points[0].latitude = 50.11;
   points[0].longitude = 8.68;
   points[0].unixTimeMs = 0;
   points[0].utcOffsetMinutes = 0;
   points[0].source = LocationHistory::PointSource::TimelinePath;
   points[1].latitude = 52.52;
   points[1].longitude = 13.40;
   points[1].unixTimeMs = 0;
   points[1].utcOffsetMinutes = 0;
   points[1].source = LocationHistory::PointSource::TimelinePath;

   LocationHistory::ClusterList highZoomClusters;
   LocationHistory::BuildClusters(points, 16, LocationHistory::ClusterCellSizePx, highZoomClusters);
   EXPECT_EQ(highZoomClusters.size(), 2u);

   LocationHistory::ClusterList lowZoomClusters;
   LocationHistory::BuildClusters(points, LocationHistory::MinZoom, 100000, lowZoomClusters);
   EXPECT_EQ(lowZoomClusters.size(), 1u);
}
