/*!
 *\file json_load_thread.h
 *\brief Background thread that loads a Timeline JSON file
 */

#ifndef JSON_LOAD_THREAD_H
#define JSON_LOAD_THREAD_H

#include <atomic>
#include <cstdint>

#include <QString>
#include <QThread>

#include "load_observer.h"
#include "load_result.h"
#include "location_data.h"

namespace LocationHistory
{
   class JsonLoadThread : public QThread, public LoadObserver
   {
         Q_OBJECT

      public:
         /*!
          *\brief Creates a worker that will load the given file when started
          *
          *\param[in] path Filesystem path to the JSON file
          *\param[in] pParent Optional Qt parent
          */
         explicit JsonLoadThread(const QString& path, QObject* pParent = nullptr);

         /*!
          *\brief Asks the running load to stop
          */
         void RequestCancel(void);

         /*!
          *\brief Returns the load result after the thread has finished
          */
         LoadResult Result(void) const;

         /*!
          *\brief Returns the file path this thread is loading
          */
         QString Path(void) const;

         /*!
          *\brief Moves the parsed points out of the worker
          *
          *\param[out] points Destination list
          */
         void TakePoints(LocationPointList& points);

         /*!
          *\brief Forwards byte progress to the UI thread
          *
          *\param[in] bytesRead Bytes consumed so far
          *\param[in] bytesTotal Total file size in bytes
          */
         void OnProgress(int64_t bytesRead, int64_t bytesTotal) override;

         /*!
          *\brief Returns true if RequestCancel has been called
          */
         bool IsCancelled(void) const override;

      signals:
         void Progress(qint64 bytesRead, qint64 bytesTotal);

      protected:
         void run(void) override;

      private:
         QString _path;
         LocationPointList _points;
         LoadResult _result;
         std::atomic<bool> _cancelled;
   };
} // namespace LocationHistory

#endif // JSON_LOAD_THREAD_H
