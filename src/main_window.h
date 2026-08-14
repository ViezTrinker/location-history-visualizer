/*!
 *\file main_window.h
 *\brief Main application window with filters, map, and About menu
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <array>
#include <cstdint>

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QTimeEdit>

#include "location_data.h"
#include "location_filter.h"
#include "map_widget.h"

namespace LocationHistory
{
   constexpr size_t WeekdayCount = 7;
   using WeekdayBoxList = std::array<QCheckBox*, WeekdayCount>;

   class MainWindow : public QMainWindow
   {
         Q_OBJECT

      public:
         explicit MainWindow(QWidget* pParent = nullptr);

      private slots:
         void OnOpenClicked(void);
         void OnAbout(void);
         void OnFiltersChanged(void);
         void OnDisplayModeChanged(int index);
         void OnHeatScaleChanged(int sliderValue);
         void OnZoomInClicked(void);
         void OnZoomOutClicked(void);
         void OnZoomSliderChanged(int zoomValue);
         void OnMapZoomChanged(int32_t zoom);
         void OnPointClicked(double latitude, double longitude, int64_t unixTimeMs, int32_t utcOffsetMinutes);
         void OnPointCleared(void);

      private:
         void BuildUi(void);
         void BuildMenus(void);
         FilterSettings ReadFilterSettings(void) const;
         void ApplyCurrentFilter(void);
         void UpdateDateRangeFromPoints(void);
         void UpdateHeatScaleControls(void);
         void SyncZoomSlider(void);

         LocationPointList _allPoints;
         LocationPointList _filteredPoints;
         MapWidget* _pMapWidget;
         QPushButton* _pOpenButton;
         QLabel* _pFileLabel;
         QDateEdit* _pFromDate;
         QDateEdit* _pToDate;
         WeekdayBoxList _pWeekdayBoxes;
         QTimeEdit* _pFromTime;
         QTimeEdit* _pToTime;
         QComboBox* _pDisplayMode;
         QSlider* _pHeatScaleSlider;
         QLabel* _pHeatScaleValueLabel;
         QPushButton* _pZoomInButton;
         QPushButton* _pZoomOutButton;
         QSlider* _pZoomSlider;
         QLabel* _pTimeLabel;
         QLabel* _pLatitudeLabel;
         QLabel* _pLongitudeLabel;
   };
} // namespace LocationHistory

#endif // MAIN_WINDOW_H
