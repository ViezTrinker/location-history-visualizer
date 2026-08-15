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
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QRect>
#include <QResizeEvent>
#include <QStandardPaths>
#include <QString>

#include "clusterer.h"
#include "location_data.h"
#include "location_point.h"
#include "map_focus.h"
#include "story_time.h"
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
      inline constexpr int32_t PathCullMarginPx = 64;
      inline constexpr int32_t PathLineWidthPx = 2;
      inline constexpr double VisitRadiusHours = 8.0;

      QString AttributionText(void)
      {
         return QString::fromUtf8(OsmAttribution.data(), static_cast<int>(OsmAttribution.size()));
      }

      int32_t VisitRadiusPx(const LocationPoint& point)
      {
         if (!PointHasDuration(point))
         {
            return VisitMinRadiusPx;
         }

         const auto durationMs = static_cast<double>(point.endUnixTimeMs - point.unixTimeMs);
         const double hours = durationMs / 3600000.0;
         double ratio = hours / VisitRadiusHours;
         if (ratio > 1.0)
         {
            ratio = 1.0;
         }
         return VisitMinRadiusPx + static_cast<int32_t>(
            ratio * static_cast<double>(VisitMaxRadiusPx - VisitMinRadiusPx));
      }
   } // namespace

   MapWidget::MapWidget(QWidget* pParent)
      : QWidget(pParent)
      , _displayMode(DisplayMode::Points)
      , _pDownloader(new TileDownloader(this))
      , _zoom(DefaultZoom)
      , _dragState(DragState::Idle)
      , _skipReleaseSelect(SkipReleaseSelect::No)
      , _selectedIndex(NoSelection)
      , _untilUnixTimeMs(ShowAllUntilTimeMs)
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
      ClearSelectionIfHidden();
      update();
   }

   void MapWidget::SetDisplayMode(const DisplayMode displayMode)
   {
      _displayMode = displayMode;
      update();
   }

   void MapWidget::SetUntilTime(const int64_t untilUnixTimeMs)
   {
      if (untilUnixTimeMs == _untilUnixTimeMs)
      {
         return;
      }

      _untilUnixTimeMs = untilUnixTimeMs;
      ClearSelectionIfHidden();
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

   bool MapWidget::IsPointVisible(const LocationPoint& point) const
   {
      if (_displayMode != DisplayMode::Story)
      {
         return true;
      }
      return PointVisibleUntil(point, _untilUnixTimeMs);
   }

   bool MapWidget::IsOnScreen(const int32_t screenX, const int32_t screenY, const int32_t marginPx) const
   {
      if (screenX < -marginPx)
      {
         return false;
      }
      if (screenX > width() + marginPx)
      {
         return false;
      }
      if (screenY < -marginPx)
      {
         return false;
      }
      if (screenY > height() + marginPx)
      {
         return false;
      }
      return true;
   }

   void MapWidget::ClearSelectionIfHidden(void)
   {
      if (_selectedIndex == NoSelection)
      {
         return;
      }
      if (_selectedIndex >= static_cast<int32_t>(_points.size()))
      {
         _selectedIndex = NoSelection;
         emit PointCleared();
         return;
      }
      if (!IsPointVisible(_points[static_cast<size_t>(_selectedIndex)]))
      {
         _selectedIndex = NoSelection;
         emit PointCleared();
      }
   }

   void MapWidget::DrawAllPoints(QPainter& painter)
   {
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor(200, 40, 40, 200));

      int32_t step = 1;
      if (_points.size() > static_cast<size_t>(MaxDrawnPoints))
      {
         step = static_cast<int32_t>(_points.size() / static_cast<size_t>(MaxDrawnPoints));
         if (step < 1)
         {
            step = 1;
         }
      }

      for (size_t index = 0; index < _points.size(); index += static_cast<size_t>(step))
      {
         const LocationPoint& point = _points[index];
         if (!IsPointVisible(point))
         {
            continue;
         }
         const int32_t screenX = WorldToScreenX(LongitudeToWorldX(point.longitude, _zoom));
         const int32_t screenY = WorldToScreenY(LatitudeToWorldY(point.latitude, _zoom));
         if (!IsOnScreen(screenX, screenY, PointRadiusPx))
         {
            continue;
         }
         painter.drawEllipse(QPoint(screenX, screenY), PointRadiusPx, PointRadiusPx);
      }
   }

   void MapWidget::DrawPaths(QPainter& painter)
   {
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setPen(QPen(QColor(210, 55, 45, 200), PathLineWidthPx));

      int32_t lastPathId = NoPathId;
      int32_t lastScreenX = 0;
      int32_t lastScreenY = 0;
      bool haveLast = false;
      for (size_t index = 0; index < _points.size(); ++index)
      {
         const LocationPoint& point = _points[index];
         if (point.source != PointSource::TimelinePath)
         {
            haveLast = false;
            continue;
         }
         if (!IsPointVisible(point))
         {
            haveLast = false;
            continue;
         }
         if (point.pathId == NoPathId)
         {
            haveLast = false;
            continue;
         }

         const int32_t screenX = WorldToScreenX(LongitudeToWorldX(point.longitude, _zoom));
         const int32_t screenY = WorldToScreenY(LatitudeToWorldY(point.latitude, _zoom));
         if (haveLast && (lastPathId == point.pathId))
         {
            if (IsOnScreen(lastScreenX, lastScreenY, PathCullMarginPx) ||
                IsOnScreen(screenX, screenY, PathCullMarginPx))
            {
               painter.drawLine(lastScreenX, lastScreenY, screenX, screenY);
            }
         }

         lastScreenX = screenX;
         lastScreenY = screenY;
         lastPathId = point.pathId;
         haveLast = true;
      }
   }

   void MapWidget::DrawTrackPoints(QPainter& painter)
   {
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setPen(Qt::NoPen);

      int32_t step = 1;
      if (_points.size() > static_cast<size_t>(MaxDrawnPoints))
      {
         step = static_cast<int32_t>(_points.size() / static_cast<size_t>(MaxDrawnPoints));
         if (step < 1)
         {
            step = 1;
         }
      }

      for (size_t index = 0; index < _points.size(); index += static_cast<size_t>(step))
      {
         const LocationPoint& point = _points[index];
         if (point.source == PointSource::Visit)
         {
            continue;
         }
         if (!IsPointVisible(point))
         {
            continue;
         }

         const int32_t screenX = WorldToScreenX(LongitudeToWorldX(point.longitude, _zoom));
         const int32_t screenY = WorldToScreenY(LatitudeToWorldY(point.latitude, _zoom));
         const int32_t radius = (point.source == PointSource::TimelinePath) ? PathPointRadiusPx : PointRadiusPx;
         if (!IsOnScreen(screenX, screenY, radius))
         {
            continue;
         }

         if (point.source == PointSource::TimelinePath)
         {
            painter.setBrush(QColor(200, 40, 40, 200));
         }
         else
         {
            painter.setBrush(QColor(90, 90, 90, 180));
         }
         painter.drawEllipse(QPoint(screenX, screenY), radius, radius);
      }
   }

   void MapWidget::DrawVisits(QPainter& painter)
   {
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setPen(QPen(QColor(20, 50, 80, 180), 1));
      painter.setBrush(QColor(30, 110, 170, 200));

      for (size_t index = 0; index < _points.size(); ++index)
      {
         const LocationPoint& point = _points[index];
         if (point.source != PointSource::Visit)
         {
            continue;
         }
         if (!IsPointVisible(point))
         {
            continue;
         }

         const int32_t screenX = WorldToScreenX(LongitudeToWorldX(point.longitude, _zoom));
         const int32_t screenY = WorldToScreenY(LatitudeToWorldY(point.latitude, _zoom));
         const int32_t radius = VisitRadiusPx(point);
         if (!IsOnScreen(screenX, screenY, radius))
         {
            continue;
         }
         painter.drawEllipse(QPoint(screenX, screenY), radius, radius);
      }
   }

   void MapWidget::DrawSelection(QPainter& painter)
   {
      if ((_selectedIndex < 0) || (_selectedIndex >= static_cast<int32_t>(_points.size())))
      {
         return;
      }

      const LocationPoint& selected = _points[static_cast<size_t>(_selectedIndex)];
      if (!IsPointVisible(selected))
      {
         return;
      }

      const int32_t screenX = WorldToScreenX(LongitudeToWorldX(selected.longitude, _zoom));
      const int32_t screenY = WorldToScreenY(LatitudeToWorldY(selected.latitude, _zoom));
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen(QColor(255, 255, 255), 2));
      painter.drawEllipse(QPoint(screenX, screenY), PointRadiusPx + 3, PointRadiusPx + 3);
      painter.setPen(QPen(QColor(30, 30, 30), 1));
      painter.drawEllipse(QPoint(screenX, screenY), PointRadiusPx + 5, PointRadiusPx + 5);
   }

   void MapWidget::DrawClusters(QPainter& painter)
   {
      LocationPointList visiblePoints;
      visiblePoints.reserve(_points.size());
      for (size_t index = 0; index < _points.size(); ++index)
      {
         if (IsPointVisible(_points[index]))
         {
            visiblePoints.push_back(_points[index]);
         }
      }

      ClusterList clusters;
      BuildClusters(visiblePoints, _zoom, ClusterCellSizePx, clusters);

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

      if (_displayMode == DisplayMode::Points)
      {
         DrawAllPoints(painter);
         DrawSelection(painter);
      }
      else if (_displayMode == DisplayMode::Story)
      {
         DrawAllPoints(painter);
         DrawSelection(painter);
      }
      else if (_displayMode == DisplayMode::Clustered)
      {
         DrawClusters(painter);
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
         if (!IsPointVisible(point))
         {
            continue;
         }

         const int32_t pointX = WorldToScreenX(LongitudeToWorldX(point.longitude, _zoom));
         const int32_t pointY = WorldToScreenY(LatitudeToWorldY(point.latitude, _zoom));
         const double deltaX = static_cast<double>(pointX - screenX);
         const double deltaY = static_cast<double>(pointY - screenY);
         const double distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
         double hitRadius = static_cast<double>(HitTestRadiusPx);
         if (point.source == PointSource::Visit)
         {
            hitRadius = static_cast<double>(VisitRadiusPx(point) + 4);
         }
         if (distance <= hitRadius)
         {
            if ((nearestIndex == NoSelection) || (distance < nearestDistance))
            {
               nearestDistance = distance;
               nearestIndex = static_cast<int32_t>(index);
            }
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
      emit PointClicked(
         point.latitude,
         point.longitude,
         point.unixTimeMs,
         point.utcOffsetMinutes,
         point.endUnixTimeMs,
         point.source);
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
