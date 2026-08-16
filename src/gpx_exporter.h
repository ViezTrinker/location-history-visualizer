/*!
 *\file gpx_exporter.h
 *\brief Writes filtered location points as GPX 1.1
 */

#ifndef GPX_EXPORTER_H
#define GPX_EXPORTER_H

#include <string>

#include "export_result.h"
#include "location_data.h"

namespace LocationHistory
{
   /*!
    *\brief Serializes location points to a GPX 1.1 document
    *
    *\param[in] points Filtered location points
    *\param[out] output GPX XML text
    */
   ExportResult WriteGpx(const LocationPointList& points, std::string& output);
} // namespace LocationHistory

#endif // GPX_EXPORTER_H
