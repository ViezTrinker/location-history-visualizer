/*!
 *\file tile_cache.h
 *\brief Memory and disk cache for OpenStreetMap raster tiles
 */

#ifndef TILE_CACHE_H
#define TILE_CACHE_H

#include <cstdint>
#include <list>
#include <string>
#include <string_view>
#include <unordered_map>

#include <QByteArray>
#include <QPixmap>

#include "tile_math.h"

namespace LocationHistory
{
   enum class CacheLookup : int8_t
   {
      Miss = -1,
      Hit = 0
   };

   inline constexpr int32_t MaxMemoryTiles = 256;

   inline bool IsOk(const CacheLookup result)
   {
      return result == CacheLookup::Hit;
   }

   inline bool IsErr(const CacheLookup result)
   {
      return result == CacheLookup::Miss;
   }

   class TileCache
   {
      public:
         TileCache(void);

         /*!
          *\brief Sets the directory used for on-disk tile storage
          *
          *\param[in] path Cache root directory
          */
         void SetCacheDirectory(std::string_view path);

         /*!
          *\brief Looks up a tile in the memory cache
          *
          *\param[in] tileId Tile coordinates
          *\param[out] pixmap Cached pixmap if present
          */
         CacheLookup TryGet(const TileId& tileId, QPixmap& pixmap);

         /*!
          *\brief Stores a tile in the memory cache
          *
          *\param[in] tileId Tile coordinates
          *\param[in] pixmap Tile image
          */
         void Store(const TileId& tileId, const QPixmap& pixmap);

         /*!
          *\brief Loads a tile from the disk cache into memory
          *
          *\param[in] tileId Tile coordinates
          *\param[out] pixmap Loaded pixmap if present
          */
         CacheLookup TryLoadFromDisk(const TileId& tileId, QPixmap& pixmap);

         /*!
          *\brief Writes PNG tile bytes to the disk cache
          *
          *\param[in] tileId Tile coordinates
          *\param[in] pngData PNG file bytes
          */
         void SaveToDisk(const TileId& tileId, const QByteArray& pngData);

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

         void Touch(const TileId& tileId);
         void EvictIfNeeded(void);
         std::string DiskPathFor(const TileId& tileId) const;

         std::string _cacheDirectory;
         std::unordered_map<TileId, QPixmap, TileIdHash> _memoryTiles;
         std::list<TileId> _lruOrder;
         std::unordered_map<TileId, std::list<TileId>::iterator, TileIdHash> _lruLookup;
   };
} // namespace LocationHistory

#endif // TILE_CACHE_H
