/*!
 *\file tile_downloader.h
 *\brief Downloads OpenStreetMap raster tiles with a concurrency limit
 */

#ifndef TILE_DOWNLOADER_H
#define TILE_DOWNLOADER_H

#include <cstdint>
#include <deque>
#include <string_view>
#include <unordered_set>

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

#include "tile_math.h"

namespace LocationHistory
{
   inline constexpr int32_t MaxConcurrentDownloads = 2;
   inline constexpr std::string_view TileUserAgent = "LocationHistoryVisualizer/2.0.0.R (https://github.com/ViezTrinker/location-history-visualizer)";
   inline constexpr std::string_view TileUrlPrefix = "https://tile.openstreetmap.org/";

   class TileDownloader : public QObject
   {
         Q_OBJECT

      public:
         explicit TileDownloader(QObject* pParent = nullptr);

         /*!
          *\brief Requests a tile download if it is not already queued or in flight
          *
          *\param[in] tileId Tile coordinates
          */
         void RequestTile(const TileId& tileId);

      signals:
         void TileDownloaded(int32_t zoom, int32_t tileX, int32_t tileY, QByteArray pngData);

      private slots:
         void OnReplyFinished(void);

      private:
         struct TileIdHash
         {
            size_t operator()(const TileId& tileId) const
            {
               const auto packedZoom = static_cast<uint64_t>(static_cast<uint32_t>(tileId.zoom));
               const auto packedX = static_cast<uint64_t>(static_cast<uint32_t>(tileId.tileX));
               const auto packedY = static_cast<uint64_t>(static_cast<uint32_t>(tileId.tileY));
               return static_cast<size_t>((packedZoom << 48) ^ (packedX << 24) ^ packedY);
            }
         };

         void StartNext(void);
         QString BuildUrl(const TileId& tileId) const;

         QNetworkAccessManager _network;
         std::deque<TileId> _queue;
         std::unordered_set<TileId, TileIdHash> _queued;
         std::unordered_set<TileId, TileIdHash> _inFlight;
         int32_t _activeCount;
   };
} // namespace LocationHistory

#endif // TILE_DOWNLOADER_H
