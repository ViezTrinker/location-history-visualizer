/*!
 *\file load_result.h
 *\brief Result codes for loading Timeline JSON files
 */

#ifndef LOAD_RESULT_H
#define LOAD_RESULT_H

#include <cstdint>

namespace LocationHistory
{
   enum class LoadResult : int8_t
   {
      FileNotFound = -4,
      InvalidJson = -3,
      NoPoints = -2,
      Error = -1,
      Ok = 0
   };

   enum class ParseResult : int8_t
   {
      InvalidFormat = -1,
      Ok = 0
   };

   /*!
    *\brief Returns true if the load result is an error
    *
    *\param[in] result Load result to check
    */
   inline bool IsErr(const LoadResult result)
   {
      return result < LoadResult::Ok;
   }

   /*!
    *\brief Returns true if the load result is success
    *
    *\param[in] result Load result to check
    */
   inline bool IsOk(const LoadResult result)
   {
      return result == LoadResult::Ok;
   }

   /*!
    *\brief Returns true if the load result is an informational message
    *
    *\param[in] result Load result to check
    */
   inline bool IsMsg(const LoadResult result)
   {
      return result > LoadResult::Ok;
   }

   /*!
    *\brief Returns true if the parse result is an error
    *
    *\param[in] result Parse result to check
    */
   inline bool IsErr(const ParseResult result)
   {
      return result < ParseResult::Ok;
   }

   /*!
    *\brief Returns true if the parse result is success
    *
    *\param[in] result Parse result to check
    */
   inline bool IsOk(const ParseResult result)
   {
      return result == ParseResult::Ok;
   }
} // namespace LocationHistory

#endif // LOAD_RESULT_H
