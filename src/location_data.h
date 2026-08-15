/*!
 *\file location_data.h
 *\brief Shared location list and map display mode types
 */

#ifndef LOCATION_DATA_H
#define LOCATION_DATA_H

#include <cstdint>
#include <vector>

#include "location_point.h"

namespace LocationHistory
{
   using LocationPointList = std::vector<LocationPoint>;

   enum class DisplayMode : uint8_t
   {
      Points = 0,
      Clustered = 1,
      Story = 2
   };
} // namespace LocationHistory

#endif // LOCATION_DATA_H
