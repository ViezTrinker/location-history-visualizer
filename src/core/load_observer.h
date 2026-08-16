/*!
 *\file load_observer.h
 *\brief Optional progress and cancellation callbacks for JSON loading
 */

#ifndef LOAD_OBSERVER_H
#define LOAD_OBSERVER_H

#include <cstdint>

namespace LocationHistory
{
   class LoadObserver
   {
      public:
         virtual ~LoadObserver(void) = default;

         /*!
          *\brief Reports how many bytes of the input have been read
          *
          *\param[in] bytesRead Bytes consumed so far
          *\param[in] bytesTotal Total file or string size in bytes
          */
         virtual void OnProgress(int64_t bytesRead, int64_t bytesTotal) = 0;

         /*!
          *\brief Returns true if the load should stop
          */
         virtual bool IsCancelled(void) const = 0;
   };
} // namespace LocationHistory

#endif // LOAD_OBSERVER_H
