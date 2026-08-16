/*!
 *\file tile_downloader.cpp
 *\brief Downloads OpenStreetMap raster tiles with a concurrency limit
 */

#include "tile_downloader.h"

#include <cstdint>
#include <string_view>

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVariant>

#include "tile_math.h"

namespace LocationHistory
{
   namespace
   {
      inline constexpr std::string_view PropertyZoom = "zoom";
      inline constexpr std::string_view PropertyTileX = "tileX";
      inline constexpr std::string_view PropertyTileY = "tileY";
   } // namespace

   TileDownloader::TileDownloader(QObject* pParent)
      : QObject(pParent)
      , _activeCount(0)
   {
      _network.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
   }

   QString TileDownloader::BuildUrl(const TileId& tileId) const
   {
      QString url = QString::fromUtf8(TileUrlPrefix.data(), static_cast<int>(TileUrlPrefix.size()));
      url += QString::number(tileId.zoom);
      url += QLatin1Char('/');
      url += QString::number(tileId.tileX);
      url += QLatin1Char('/');
      url += QString::number(tileId.tileY);
      url += QStringLiteral(".png");
      return url;
   }

   void TileDownloader::RequestTile(const TileId& tileId)
   {
      if (_queued.find(tileId) != _queued.end())
      {
         return;
      }
      if (_inFlight.find(tileId) != _inFlight.end())
      {
         return;
      }

      _queue.push_back(tileId);
      _queued.insert(tileId);
      StartNext();
   }

   void TileDownloader::StartNext(void)
   {
      while ((_activeCount < MaxConcurrentDownloads) && !_queue.empty())
      {
         const TileId tileId = _queue.front();
         _queue.pop_front();
         _queued.erase(tileId);
         _inFlight.insert(tileId);

         QNetworkRequest request(QUrl(BuildUrl(tileId)));
         request.setHeader(
            QNetworkRequest::UserAgentHeader,
            QString::fromUtf8(TileUserAgent.data(), static_cast<int>(TileUserAgent.size())));
         request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

         QNetworkReply* pReply = _network.get(request);
         pReply->setProperty(PropertyZoom.data(), tileId.zoom);
         pReply->setProperty(PropertyTileX.data(), tileId.tileX);
         pReply->setProperty(PropertyTileY.data(), tileId.tileY);
         connect(pReply, &QNetworkReply::finished, this, &TileDownloader::OnReplyFinished);
         _activeCount += 1;
      }
   }

   void TileDownloader::OnReplyFinished(void)
   {
      QNetworkReply* pReply = qobject_cast<QNetworkReply*>(sender());
      if (pReply == nullptr)
      {
         return;
      }

      TileId tileId{};
      tileId.zoom = pReply->property(PropertyZoom.data()).toInt();
      tileId.tileX = pReply->property(PropertyTileX.data()).toInt();
      tileId.tileY = pReply->property(PropertyTileY.data()).toInt();

      _inFlight.erase(tileId);
      if (_activeCount > 0)
      {
         _activeCount -= 1;
      }

      if (pReply->error() == QNetworkReply::NoError)
      {
         const QByteArray pngData = pReply->readAll();
         if (!pngData.isEmpty())
         {
            emit TileDownloaded(tileId.zoom, tileId.tileX, tileId.tileY, pngData);
         }
      }

      pReply->deleteLater();
      StartNext();
   }
} // namespace LocationHistory
