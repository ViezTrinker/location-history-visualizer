/*!
 *\file tile_cache.cpp
 *\brief Memory and disk cache for OpenStreetMap raster tiles
 */

#include "tile_cache.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include <QByteArray>
#include <QFile>
#include <QPixmap>

#include "tile_math.h"

namespace LocationHistory
{
   namespace
   {
      void AppendIntPath(std::string& path, const int32_t value)
      {
         path += std::to_string(value);
      }
   } // namespace

   TileCache::TileCache(void)
   {
   }

   void TileCache::SetCacheDirectory(const std::string_view path)
   {
      _cacheDirectory = std::string(path);
      if (_cacheDirectory.empty())
      {
         return;
      }

      std::error_code errorCode;
      std::filesystem::create_directories(_cacheDirectory, errorCode);
   }

   void TileCache::Touch(const TileId& tileId)
   {
      const auto lookup = _lruLookup.find(tileId);
      if (lookup != _lruLookup.end())
      {
         _lruOrder.erase(lookup->second);
      }
      _lruOrder.push_front(tileId);
      _lruLookup[tileId] = _lruOrder.begin();
   }

   void TileCache::EvictIfNeeded(void)
   {
      while (_memoryTiles.size() > static_cast<size_t>(MaxMemoryTiles))
      {
         if (_lruOrder.empty())
         {
            return;
         }

         const TileId oldest = _lruOrder.back();
         _lruOrder.pop_back();
         _lruLookup.erase(oldest);
         _memoryTiles.erase(oldest);
      }
   }

   std::string TileCache::DiskPathFor(const TileId& tileId) const
   {
      std::string path = _cacheDirectory;
      if (!path.empty())
      {
         path.push_back('/');
      }
      AppendIntPath(path, tileId.zoom);
      path.push_back('/');
      AppendIntPath(path, tileId.tileX);
      path.push_back('/');
      AppendIntPath(path, tileId.tileY);
      path += ".png";
      return path;
   }

   CacheLookup TileCache::TryGet(const TileId& tileId, QPixmap& pixmap)
   {
      const auto found = _memoryTiles.find(tileId);
      if (found == _memoryTiles.end())
      {
         return CacheLookup::Miss;
      }

      pixmap = found->second;
      Touch(tileId);
      return CacheLookup::Hit;
   }

   void TileCache::Store(const TileId& tileId, const QPixmap& pixmap)
   {
      _memoryTiles[tileId] = pixmap;
      Touch(tileId);
      EvictIfNeeded();
   }

   CacheLookup TileCache::TryLoadFromDisk(const TileId& tileId, QPixmap& pixmap)
   {
      if (_cacheDirectory.empty())
      {
         return CacheLookup::Miss;
      }

      const std::string path = DiskPathFor(tileId);
      QPixmap loaded;
      if (!loaded.load(QString::fromStdString(path)))
      {
         return CacheLookup::Miss;
      }

      Store(tileId, loaded);
      pixmap = loaded;
      return CacheLookup::Hit;
   }

   void TileCache::SaveToDisk(const TileId& tileId, const QByteArray& pngData)
   {
      if (_cacheDirectory.empty())
      {
         return;
      }
      if (pngData.isEmpty())
      {
         return;
      }

      const std::string path = DiskPathFor(tileId);
      const std::filesystem::path filePath(path);
      std::error_code errorCode;
      std::filesystem::create_directories(filePath.parent_path(), errorCode);

      QFile file(QString::fromStdString(path));
      if (!file.open(QIODevice::WriteOnly))
      {
         return;
      }
      file.write(pngData);
      file.close();
   }
} // namespace LocationHistory
