/*!
 *\file json_load_thread.cpp
 *\brief Background thread that loads a Timeline JSON file
 */

#include "json_load_thread.h"

#include <string>

#include "json_loader.h"
#include "load_result.h"
#include "location_data.h"

namespace LocationHistory
{
   JsonLoadThread::JsonLoadThread(const QString& path, QObject* pParent)
      : QThread(pParent)
      , _path(path)
      , _result(LoadResult::Error)
      , _cancelled(false)
   {
   }

   void JsonLoadThread::RequestCancel(void)
   {
      _cancelled.store(true);
   }

   LoadResult JsonLoadThread::Result(void) const
   {
      return _result;
   }

   QString JsonLoadThread::Path(void) const
   {
      return _path;
   }

   void JsonLoadThread::TakePoints(LocationPointList& points)
   {
      points.swap(_points);
   }

   void JsonLoadThread::OnProgress(const int64_t bytesRead, const int64_t bytesTotal)
   {
      emit Progress(bytesRead, bytesTotal);
   }

   bool JsonLoadThread::IsCancelled(void) const
   {
      return _cancelled.load();
   }

   void JsonLoadThread::run(void)
   {
      const std::string pathString = _path.toStdString();
      _result = LoadFromFile(pathString, _points, this);
   }
} // namespace LocationHistory
