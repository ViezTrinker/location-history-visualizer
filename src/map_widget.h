/*!
 *\file map_widget.h
 *\brief Interactive OSM map widget with location overlays
 */

#ifndef MAP_WIDGET_H
#define MAP_WIDGET_H

#include <cstdint>
#include <string_view>

#include <QByteArray>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QString>
#include <QWheelEvent>
#include <QWidget>

#include "location_data.h"
#include "tile_cache.h"
#include "tile_downloader.h"
#include "tile_math.h"

namespace LocationHistory
{
   inline constexpr double DefaultLatitude = 51.29;
   inline constexpr double DefaultLongitude = 9.45;
   inline constexpr int32_t DefaultZoom = 13;
   inline constexpr int32_t HitTestRadiusPx = 12;
   inline constexpr int32_t VisitMinRadiusPx = 7;
   inline constexpr int32_t VisitMaxRadiusPx = 16;
   inline constexpr int32_t NoSelection = -1;
   inline constexpr int32_t AttributionPaddingPx = 8;
   inline constexpr std::string_view OsmAttribution = "© OpenStreetMap contributors";

   class MapWidget : public QWidget
   {
         Q_OBJECT

      public:
         explicit MapWidget(QWidget* pParent = nullptr);

         /*!
          *\brief Replaces the points drawn on the map
          *
          *\param[in] points Filtered location points
          */
         void SetPoints(const LocationPointList& points);

         /*!
          *\brief Sets the overlay display mode
          *
          *\param[in] displayMode Visualization mode
          */
         void SetDisplayMode(DisplayMode displayMode);

         /*!
          *\brief Limits drawing to samples that started at or before this time
          *
          *\param[in] untilUnixTimeMs Inclusive story cutoff
          */
         void SetUntilTime(int64_t untilUnixTimeMs);

         /*!
          *\brief Sets how many location circles may be painted
          *
          *\param[in] drawnPointLimit Maximum number of drawn points
          */
         void SetDrawnPointLimit(int32_t drawnPointLimit);

         /*!
          *\brief Returns the current drawn-point limit
          */
         int32_t DrawnPointLimit(void) const;

         /*!
          *\brief Sets the radius of location circles in pixels
          *
          *\param[in] pointRadiusPx Circle radius
          */
         void SetPointRadiusPx(int32_t pointRadiusPx);

         /*!
          *\brief Returns the current location circle radius in pixels
          */
         int32_t PointRadiusPx(void) const;

         /*!
          *\brief Centers the map on the densest cluster of the current points
          */
         void CenterOnPoints(void);

         /*!
          *\brief Returns the current OSM zoom level
          */
         int32_t Zoom(void) const;

         /*!
          *\brief Zooms in around the map center
          */
         void ZoomIn(void);

         /*!
          *\brief Zooms out around the map center
          */
         void ZoomOut(void);

         /*!
          *\brief Sets the zoom level around the map center
          *
          *\param[in] zoom Requested zoom level
          */
         void SetZoomLevel(int32_t zoom);

      signals:
         void PointClicked(
            double latitude,
            double longitude,
            int64_t unixTimeMs,
            int32_t utcOffsetMinutes,
            int64_t endUnixTimeMs,
            PointSource source);
         void PointCleared(void);
         void ZoomChanged(int32_t zoom);

      private slots:
         void OnTileDownloaded(int32_t zoom, int32_t tileX, int32_t tileY, QByteArray pngData);

      protected:
         void paintEvent(QPaintEvent* pEvent) override;
         void mousePressEvent(QMouseEvent* pEvent) override;
         void mouseMoveEvent(QMouseEvent* pEvent) override;
         void mouseReleaseEvent(QMouseEvent* pEvent) override;
         void mouseDoubleClickEvent(QMouseEvent* pEvent) override;
         void wheelEvent(QWheelEvent* pEvent) override;
         void resizeEvent(QResizeEvent* pEvent) override;

      private:
         enum class DragState : uint8_t
         {
            Idle = 0,
            Panning = 1
         };

         enum class SkipReleaseSelect : uint8_t
         {
            No = 0,
            Yes = 1
         };

         double ScreenToWorldX(int32_t screenX) const;
         double ScreenToWorldY(int32_t screenY) const;
         int32_t WorldToScreenX(double worldX) const;
         int32_t WorldToScreenY(double worldY) const;
         void SetZoomAround(int32_t newZoom, int32_t anchorScreenX, int32_t anchorScreenY);
         void DrawTiles(QPainter& painter);
         void DrawAllPoints(QPainter& painter);
         void DrawPaths(QPainter& painter);
         void DrawTrackPoints(QPainter& painter);
         void DrawVisits(QPainter& painter);
         void DrawSelection(QPainter& painter);
         void DrawClusters(QPainter& painter);
         void DrawAttribution(QPainter& painter);
         CacheLookup EnsureTile(const TileId& tileId, QPixmap& pixmap);
         bool IsPointVisible(const LocationPoint& point) const;
         bool IsOnScreen(int32_t screenX, int32_t screenY, int32_t marginPx) const;
         int32_t FindNearestPoint(int32_t screenX, int32_t screenY) const;
         void SelectPointAt(int32_t screenX, int32_t screenY);
         void ClearSelectionIfHidden(void);

         LocationPointList _points;
         DisplayMode _displayMode;
         TileCache _tileCache;
         TileDownloader* _pDownloader;
         double _centerWorldX;
         double _centerWorldY;
         int32_t _zoom;
         DragState _dragState;
         SkipReleaseSelect _skipReleaseSelect;
         QPoint _lastMousePosition;
         QPoint _pressMousePosition;
         int32_t _selectedIndex;
         int64_t _untilUnixTimeMs;
         int32_t _drawnPointLimit;
         int32_t _pointRadiusPx;
   };
} // namespace LocationHistory

#endif // MAP_WIDGET_H
