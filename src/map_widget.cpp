/*!
 *\file map_widget.cpp
 *\brief Interactive OSM map widget with location overlays
 */

#include "map_widget.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QRect>
#include <QResizeEvent>
#include <QStandardPaths>
#include <QString>

#include "clusterer.h"
#include "heatmap_renderer.h"
#include "location_data.h"
#include "map_focus.h"
#include "tile_cache.h"
#include "tile_downloader.h"
#include "tile_math.h"

namespace LocationHistory
{
   namespace
   {
      inline constexpr int32_t DragClickMaxDeltaPx = 4;
      inline constexpr int32_t ClusterMinRadiusPx = 12;
      inline constexpr int32_t ClusterMaxRadiusPx = 40;

      QString AttributionText(void)
      {
         return QString::fromUtf8(OsmAttribution.data(), static_cast<int>(OsmAttribution.size()));
      }
   } // namespace

   MapWidget::MapWidget(QWidget* pParent)
      : QWidget(pParent)
      , _displayMode(DisplayMode::AllPoints)
      , _pDownloader(new TileDownloader(this))
      , _zoom(DefaultZoom)
      , _dragState(DragState::Idle)
      , _skipReleaseSelect(SkipReleaseSelect::No)
      , _selectedIndex(NoSelection)
      , _heatScale(HeatScaleDefault)
      , _pointsRevision(0)
      , _heatCacheDownWidth(0)
      , _heatCacheDownHeight(0)
      , _heatCacheBlurRadius(-1)
      , _heatCacheZoom(-1)
      , _heatCacheCenterX(0.0)
      , _heatCacheCenterY(0.0)
      , _heatCachePointsRevision(0)
   {
      setMouseTracking(true);
      setMinimumSize(400, 300);
      _centerWorldX = LongitudeToWorldX(DefaultLongitude, _zoom);
      _centerWorldY = LatitudeToWorldY(DefaultLatitude, _zoom);

      const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
      const QString tileDir = cacheRoot + QStringLiteral("/tiles");
      _tileCache.SetCacheDirectory(tileDir.toStdString());

      connect(_pDownloader, &TileDownloader::TileDownloaded, this, &MapWidget::OnTileDownloaded);
   }

   void MapWidget::SetPoints(const LocationPointList& points)
   {
      _points = points;
      _selectedIndex = NoSelection;
      _pointsRevision += 1;
      update();
   }

   void MapWidget::SetDisplayMode(const DisplayMode displayMode)
   {
      _displayMode = displayMode;
      update();
   }

   void MapWidget::SetHeatScale(const float heatScale)
   {
      float clampedScale = heatScale;
      if (clampedScale < HeatScaleMin)
      {
         clampedScale = HeatScaleMin;
      }
      if (clampedScale > HeatScaleMax)
      {
         clampedScale = HeatScaleMax;
      }

      _heatScale = clampedScale;
      update();
   }

   void MapWidget::CenterOnPoints(void)
   {
      MapFocus focus{};
      if (IsErr(ComputeDensestFocus(_points, width(), height(), focus)))
      {
         return;
      }

      _zoom = ClampZoom(focus.zoom);
      _centerWorldX = LongitudeToWorldX(focus.longitude, _zoom);
      _centerWorldY = LatitudeToWorldY(focus.latitude, _zoom);
      emit ZoomChanged(_zoom);
      update();
   }

   int32_t MapWidget::Zoom(void) const
   {
      return _zoom;
   }

   void MapWidget::ZoomIn(void)
   {
      SetZoomAround(_zoom + 1, width() / 2, height() / 2);
   }

   void MapWidget::ZoomOut(void)
   {
      SetZoomAround(_zoom - 1, width() / 2, height() / 2);
   }

   void MapWidget::SetZoomLevel(const int32_t zoom)
   {
      SetZoomAround(zoom, width() / 2, height() / 2);
   }

   void MapWidget::SetZoomAround(const int32_t newZoom, const int32_t anchorScreenX, const int32_t anchorScreenY)
   {
      const int32_t clampedZoom = ClampZoom(newZoom);
      if (clampedZoom == _zoom)
      {
         return;
      }

      const double worldX = ScreenToWorldX(anchorScreenX);
      const double worldY = ScreenToWorldY(anchorScreenY);
      const double scale = std::pow(2.0, static_cast<double>(clampedZoom - _zoom));
      _zoom = clampedZoom;
      _centerWorldX = worldX * scale - static_cast<double>(anchorScreenX) + static_cast<double>(width()) / 2.0;
      _centerWorldY = worldY * scale - static_cast<double>(anchorScreenY) + static_cast<double>(height()) / 2.0;
      emit ZoomChanged(_zoom);
      update();
   }

   double MapWidget::ScreenToWorldX(const int32_t screenX) const
   {
      return _centerWorldX - static_cast<double>(width()) / 2.0 + static_cast<double>(screenX);
   }

   double MapWidget::ScreenToWorldY(const int32_t screenY) const
   {
      return _centerWorldY - static_cast<double>(height()) / 2.0 + static_cast<double>(screenY);
   }

   int32_t MapWidget::WorldToScreenX(const double worldX) const
   {
      return static_cast<int32_t>(worldX - _centerWorldX + static_cast<double>(width()) / 2.0);
   }

   int32_t MapWidget::WorldToScreenY(const double worldY) const
   {
      return static_cast<int32_t>(worldY - _centerWorldY + static_cast<double>(height()) / 2.0);
   }

   CacheLookup MapWidget::EnsureTile(const TileId& tileId, QPixmap& pixmap)
   {
      if (IsOk(_tileCache.TryGet(tileId, pixmap)))
      {
         return CacheLookup::Hit;
      }
      if (IsOk(_tileCache.TryLoadFromDisk(tileId, pixmap)))
      {
         return CacheLookup::Hit;
      }

      _pDownloader->RequestTile(tileId);
      return CacheLookup::Miss;
   }

   void MapWidget::DrawTiles(QPainter& painter)
   {
      const int32_t tileCount = static_cast<int32_t>(static_cast<int64_t>(1) << _zoom);
      const auto minTileX = static_cast<int32_t>(std::floor(ScreenToWorldX(0) / static_cast<double>(TileSizePx)));
      const auto maxTileX = static_cast<int32_t>(std::floor(ScreenToWorldX(width()) / static_cast<double>(TileSizePx)));
      const auto minTileY = static_cast<int32_t>(std::floor(ScreenToWorldY(0) / static_cast<double>(TileSizePx)));
      const auto maxTileY = static_cast<int32_t>(std::floor(ScreenToWorldY(height()) / static_cast<double>(TileSizePx)));

      for (int32_t tileY = minTileY; tileY <= maxTileY; ++tileY)
      {
         if ((tileY < 0) || (tileY >= tileCount))
         {
            continue;
         }
         for (int32_t tileX = minTileX; tileX <= maxTileX; ++tileX)
         {
            if ((tileX < 0) || (tileX >= tileCount))
            {
               continue;
            }

            TileId tileId{};
            tileId.zoom = _zoom;
            tileId.tileX = tileX;
            tileId.tileY = tileY;

            const int32_t screenX = WorldToScreenX(static_cast<double>(tileX * TileSizePx));
            const int32_t screenY = WorldToScreenY(static_cast<double>(tileY * TileSizePx));
            QPixmap pixmap;
            if (IsOk(EnsureTile(tileId, pixmap)))
            {
               painter.drawPixmap(screenX, screenY, pixmap);
            }
            else
            {
               painter.fillRect(screenX, screenY, TileSizePx, TileSizePx, QColor(220, 220, 220));
            }
         }
      }
   }

   void MapWidget::DrawPoints(QPainter& painter)
   {
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor(220, 40, 40, 180));

      int32_t step = 1;
      if (_points.size() > static_cast<size_t>(MaxDrawnPoints))
      {
         step = static_cast<int32_t>(_points.size() / static_cast<size_t>(MaxDrawnPoints));
         if (step < 1)
         {
            step = 1;
         }
      }

      const int32_t viewWidth = width();
      const int32_t viewHeight = height();
      for (size_t index = 0; index < _points.size(); index += static_cast<size_t>(step))
      {
         const LocationPoint& point = _points[index];
         const int32_t screenX = WorldToScreenX(LongitudeToWorldX(point.longitude, _zoom));
         const int32_t screenY = WorldToScreenY(LatitudeToWorldY(point.latitude, _zoom));
         if ((screenX < -PointRadiusPx) || (screenX > viewWidth + PointRadiusPx))
         {
            continue;
         }
         if ((screenY < -PointRadiusPx) || (screenY > viewHeight + PointRadiusPx))
         {
            continue;
         }
         painter.drawEllipse(QPoint(screenX, screenY), PointRadiusPx, PointRadiusPx);
      }

      if ((_selectedIndex >= 0) && (_selectedIndex < static_cast<int32_t>(_points.size())))
      {
         const LocationPoint& selected = _points[static_cast<size_t>(_selectedIndex)];
         const int32_t screenX = WorldToScreenX(LongitudeToWorldX(selected.longitude, _zoom));
         const int32_t screenY = WorldToScreenY(LatitudeToWorldY(selected.latitude, _zoom));
         painter.setBrush(Qt::NoBrush);
         painter.setPen(QPen(QColor(255, 255, 255), 2));
         painter.drawEllipse(QPoint(screenX, screenY), PointRadiusPx + 3, PointRadiusPx + 3);
         painter.setPen(QPen(QColor(30, 30, 30), 1));
         painter.drawEllipse(QPoint(screenX, screenY), PointRadiusPx + 5, PointRadiusPx + 5);
      }
   }

   void MapWidget::DrawClusters(QPainter& painter)
   {
      ClusterList clusters;
      BuildClusters(_points, _zoom, ClusterCellSizePx, clusters);

      int32_t maxCount = 1;
      for (size_t index = 0; index < clusters.size(); ++index)
      {
         maxCount = std::max(maxCount, clusters[index].count);
      }

      painter.setRenderHint(QPainter::Antialiasing, true);
      QFont font = painter.font();
      font.setBold(true);
      painter.setFont(font);

      for (size_t index = 0; index < clusters.size(); ++index)
      {
         const Cluster& cluster = clusters[index];
         const int32_t screenX = WorldToScreenX(LongitudeToWorldX(cluster.longitude, _zoom));
         const int32_t screenY = WorldToScreenY(LatitudeToWorldY(cluster.latitude, _zoom));
         if ((screenX < -ClusterMaxRadiusPx) || (screenX > width() + ClusterMaxRadiusPx))
         {
            continue;
         }
         if ((screenY < -ClusterMaxRadiusPx) || (screenY > height() + ClusterMaxRadiusPx))
         {
            continue;
         }

         const double countRatio = std::log(static_cast<double>(cluster.count) + 1.0) / std::log(static_cast<double>(maxCount) + 1.0);
         const auto radius = static_cast<int32_t>(
            static_cast<double>(ClusterMinRadiusPx) +
            (static_cast<double>(ClusterMaxRadiusPx - ClusterMinRadiusPx) * countRatio));

         painter.setPen(QPen(QColor(20, 20, 20, 180), 1));
         painter.setBrush(QColor(30, 90, 200, 170));
         painter.drawEllipse(QPoint(screenX, screenY), radius, radius);
         painter.setPen(QColor(255, 255, 255));
         painter.drawText(
            QRect(screenX - radius, screenY - radius, radius * 2, radius * 2),
            Qt::AlignCenter,
            QString::number(cluster.count));
      }
   }

   void MapWidget::DrawHeatmap(QPainter& painter)
   {
      const auto blurRadius = static_cast<int32_t>(
         std::lround(HeatmapSigmaPx / static_cast<double>(HeatmapDownsample) * 2.0));
      DrawIntensityOverlay(painter, std::max(blurRadius, 1));
   }

   void MapWidget::DrawBlur(QPainter& painter)
   {
      const auto blurRadius = static_cast<int32_t>(
         std::lround(static_cast<double>(BlurRadiusPx) / static_cast<double>(HeatmapDownsample) * 2.0));
      DrawIntensityOverlay(painter, std::max(blurRadius, 1));
   }

   bool MapWidget::IsHeatCacheCurrent(const int32_t downWidth, const int32_t downHeight, const int32_t blurRadius) const
   {
      if (_cachedHeatIntensity.empty())
      {
         return false;
      }
      if (_heatCacheDownWidth != downWidth)
      {
         return false;
      }
      if (_heatCacheDownHeight != downHeight)
      {
         return false;
      }
      if (_heatCacheBlurRadius != blurRadius)
      {
         return false;
      }
      if (_heatCacheZoom != _zoom)
      {
         return false;
      }
      if (_heatCacheCenterX != _centerWorldX)
      {
         return false;
      }
      if (_heatCacheCenterY != _centerWorldY)
      {
         return false;
      }
      if (_heatCachePointsRevision != _pointsRevision)
      {
         return false;
      }
      return true;
   }

   void MapWidget::RebuildHeatIntensity(const int32_t viewWidth, const int32_t viewHeight, const int32_t blurRadius)
   {
      const int32_t downWidth = std::max(1, viewWidth / HeatmapDownsample);
      const int32_t downHeight = std::max(1, viewHeight / HeatmapDownsample);
      const double scaleX = static_cast<double>(downWidth) / static_cast<double>(viewWidth);
      const double scaleY = static_cast<double>(downHeight) / static_cast<double>(viewHeight);

      HeatBuffer density(static_cast<size_t>(downWidth) * static_cast<size_t>(downHeight), 0.0f);
      for (size_t index = 0; index < _points.size(); ++index)
      {
         const LocationPoint& point = _points[index];
         const double screenX = static_cast<double>(WorldToScreenX(LongitudeToWorldX(point.longitude, _zoom)));
         const double screenY = static_cast<double>(WorldToScreenY(LatitudeToWorldY(point.latitude, _zoom)));
         if ((screenX < 0.0) || (screenX > static_cast<double>(viewWidth)))
         {
            continue;
         }
         if ((screenY < 0.0) || (screenY > static_cast<double>(viewHeight)))
         {
            continue;
         }
         AddHeatSample(density, downWidth, downHeight, screenX * scaleX, screenY * scaleY, 1.0f);
      }

      GaussianBlur(density, _cachedHeatIntensity, downWidth, downHeight, blurRadius);
      _heatCacheDownWidth = downWidth;
      _heatCacheDownHeight = downHeight;
      _heatCacheBlurRadius = blurRadius;
      _heatCacheZoom = _zoom;
      _heatCacheCenterX = _centerWorldX;
      _heatCacheCenterY = _centerWorldY;
      _heatCachePointsRevision = _pointsRevision;
   }

   void MapWidget::DrawIntensityOverlay(QPainter& painter, const int32_t blurRadius)
   {
      const int32_t viewWidth = width();
      const int32_t viewHeight = height();
      if ((viewWidth <= 0) || (viewHeight <= 0))
      {
         return;
      }

      const int32_t downWidth = std::max(1, viewWidth / HeatmapDownsample);
      const int32_t downHeight = std::max(1, viewHeight / HeatmapDownsample);
      if (!IsHeatCacheCurrent(downWidth, downHeight, blurRadius))
      {
         RebuildHeatIntensity(viewWidth, viewHeight, blurRadius);
      }
      if (_cachedHeatIntensity.empty())
      {
         return;
      }

      ArgbBuffer pixels;
      HeatBufferToArgb(_cachedHeatIntensity, pixels, ScaledHeatCeiling(MaxHeat(_cachedHeatIntensity), _heatScale));
      QImage image(
         reinterpret_cast<const uchar*>(pixels.data()),
         _heatCacheDownWidth,
         _heatCacheDownHeight,
         _heatCacheDownWidth * static_cast<int32_t>(sizeof(uint32_t)),
         QImage::Format_ARGB32);
      painter.drawImage(QRect(0, 0, viewWidth, viewHeight), image);
   }

   void MapWidget::DrawAttribution(QPainter& painter)
   {
      const QString text = AttributionText();
      const QFontMetrics metrics(painter.font());
      const int32_t textWidth = metrics.horizontalAdvance(text);
      const int32_t textHeight = metrics.height();
      const int32_t boxX = AttributionPaddingPx;
      const int32_t boxY = height() - textHeight - AttributionPaddingPx * 2;
      painter.fillRect(boxX, boxY, textWidth + AttributionPaddingPx * 2, textHeight + AttributionPaddingPx, QColor(255, 255, 255, 200));
      painter.setPen(QColor(40, 40, 40));
      painter.drawText(boxX + AttributionPaddingPx, boxY + textHeight, text);
   }

   void MapWidget::paintEvent(QPaintEvent* pEvent)
   {
      (void)pEvent;
      QPainter painter(this);
      painter.fillRect(rect(), QColor(200, 200, 200));
      DrawTiles(painter);

      if (_displayMode == DisplayMode::AllPoints)
      {
         DrawPoints(painter);
      }
      else if (_displayMode == DisplayMode::Clustered)
      {
         DrawClusters(painter);
      }
      else if (_displayMode == DisplayMode::Heatmap)
      {
         DrawHeatmap(painter);
      }
      else if (_displayMode == DisplayMode::Blur)
      {
         DrawBlur(painter);
      }

      DrawAttribution(painter);
   }

   void MapWidget::mousePressEvent(QMouseEvent* pEvent)
   {
      if (pEvent->button() != Qt::LeftButton)
      {
         return;
      }

      _dragState = DragState::Panning;
      _lastMousePosition = pEvent->pos();
      _pressMousePosition = pEvent->pos();
   }

   void MapWidget::mouseMoveEvent(QMouseEvent* pEvent)
   {
      if (_dragState != DragState::Panning)
      {
         return;
      }

      const QPoint delta = pEvent->pos() - _lastMousePosition;
      _centerWorldX -= static_cast<double>(delta.x());
      _centerWorldY -= static_cast<double>(delta.y());
      _lastMousePosition = pEvent->pos();
      update();
   }

   void MapWidget::mouseReleaseEvent(QMouseEvent* pEvent)
   {
      if (pEvent->button() != Qt::LeftButton)
      {
         return;
      }

      const QPoint delta = pEvent->pos() - _pressMousePosition;
      _dragState = DragState::Idle;
      if (_skipReleaseSelect == SkipReleaseSelect::Yes)
      {
         _skipReleaseSelect = SkipReleaseSelect::No;
         return;
      }
      if ((std::abs(delta.x()) <= DragClickMaxDeltaPx) && (std::abs(delta.y()) <= DragClickMaxDeltaPx))
      {
         SelectPointAt(pEvent->pos().x(), pEvent->pos().y());
      }
   }

   void MapWidget::mouseDoubleClickEvent(QMouseEvent* pEvent)
   {
      if (pEvent->button() != Qt::LeftButton)
      {
         return;
      }

      _skipReleaseSelect = SkipReleaseSelect::Yes;
      _dragState = DragState::Idle;
      SetZoomAround(_zoom + 1, pEvent->pos().x(), pEvent->pos().y());
   }

   void MapWidget::wheelEvent(QWheelEvent* pEvent)
   {
      const int32_t delta = pEvent->angleDelta().y();
      if (delta == 0)
      {
         return;
      }

      int32_t newZoom = _zoom;
      if (delta > 0)
      {
         newZoom += 1;
      }
      else
      {
         newZoom -= 1;
      }

      const int32_t cursorX = static_cast<int32_t>(pEvent->position().x());
      const int32_t cursorY = static_cast<int32_t>(pEvent->position().y());
      SetZoomAround(newZoom, cursorX, cursorY);
   }

   void MapWidget::resizeEvent(QResizeEvent* pEvent)
   {
      (void)pEvent;
      update();
   }

   int32_t MapWidget::FindNearestPoint(const int32_t screenX, const int32_t screenY) const
   {
      int32_t nearestIndex = NoSelection;
      double nearestDistance = static_cast<double>(HitTestRadiusPx);
      for (size_t index = 0; index < _points.size(); ++index)
      {
         const LocationPoint& point = _points[index];
         const int32_t pointX = WorldToScreenX(LongitudeToWorldX(point.longitude, _zoom));
         const int32_t pointY = WorldToScreenY(LatitudeToWorldY(point.latitude, _zoom));
         const double deltaX = static_cast<double>(pointX - screenX);
         const double deltaY = static_cast<double>(pointY - screenY);
         const double distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
         if (distance <= nearestDistance)
         {
            nearestDistance = distance;
            nearestIndex = static_cast<int32_t>(index);
         }
      }
      return nearestIndex;
   }

   void MapWidget::SelectPointAt(const int32_t screenX, const int32_t screenY)
   {
      const int32_t nearestIndex = FindNearestPoint(screenX, screenY);
      _selectedIndex = nearestIndex;
      if (nearestIndex == NoSelection)
      {
         emit PointCleared();
         update();
         return;
      }

      const LocationPoint& point = _points[static_cast<size_t>(nearestIndex)];
      emit PointClicked(point.latitude, point.longitude, point.unixTimeMs, point.utcOffsetMinutes);
      update();
   }

   void MapWidget::OnTileDownloaded(const int32_t zoom, const int32_t tileX, const int32_t tileY, const QByteArray pngData)
   {
      QPixmap pixmap;
      if (!pixmap.loadFromData(pngData, "PNG"))
      {
         return;
      }

      TileId tileId{};
      tileId.zoom = zoom;
      tileId.tileX = tileX;
      tileId.tileY = tileY;
      _tileCache.SaveToDisk(tileId, pngData);
      _tileCache.Store(tileId, pixmap);
      if (zoom == _zoom)
      {
         update();
      }
   }
} // namespace LocationHistory
