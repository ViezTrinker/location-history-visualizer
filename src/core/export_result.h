/*!
 *\file export_result.h
 *\brief Result codes for exporting location points
 */

#ifndef EXPORT_RESULT_H
#define EXPORT_RESULT_H

#include <cstdint>

namespace LocationHistory
{
   enum class ExportResult : int8_t
   {
      WriteFailed = -2,
      NoPoints = -1,
      Ok = 0
   };

   /*!
    *\brief Returns true if the export result is an error
    *
    *\param[in] result Export result to check
    */
   inline bool IsErr(const ExportResult result)
   {
      return result < ExportResult::Ok;
   }

   /*!
    *\brief Returns true if the export result is success
    *
    *\param[in] result Export result to check
    */
   inline bool IsOk(const ExportResult result)
   {
      return result == ExportResult::Ok;
   }
} // namespace LocationHistory

#endif // EXPORT_RESULT_H
