/*!
 *\file main_window.h
 *\brief Main application window with filters, map, and About menu
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <array>
#include <cstdint>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QTimeEdit>

#include "load_result.h"
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
         void OnLanguageChanged(int index);
         void OnPointClicked(double latitude, double longitude, int64_t unixTimeMs, int32_t utcOffsetMinutes);
         void OnPointCleared(void);

      protected:
         void changeEvent(QEvent* pEvent) override;

      private:
         void BuildUi(void);
         void BuildMenus(void);
         void RetranslateUi(void);
         void FillLanguageCombo(void);
         void UpdateFileLabel(void);
         void UpdateStatusMessage(void);
         QString WeekdayText(size_t weekdayIndex) const;
         QString LoadResultMessage(LoadResult result) const;
         FilterSettings ReadFilterSettings(void) const;
         void ApplyCurrentFilter(void);
         void UpdateDateRangeFromPoints(void);
         void UpdateHeatScaleControls(void);
         void SyncZoomSlider(void);

         LocationPointList _allPoints;
         LocationPointList _filteredPoints;
         QString _loadedFilePath;
         MapWidget* _pMapWidget;
         QMenu* _pFileMenu;
         QAction* _pOpenAction;
         QAction* _pQuitAction;
         QMenu* _pHelpMenu;
         QAction* _pAboutAction;
         QPushButton* _pOpenButton;
         QLabel* _pFileLabel;
         QGroupBox* _pDateGroup;
         QLabel* _pFromDateLabel;
         QLabel* _pToDateLabel;
         QDateEdit* _pFromDate;
         QDateEdit* _pToDate;
         QGroupBox* _pWeekdayGroup;
         WeekdayBoxList _pWeekdayBoxes;
         QGroupBox* _pTimeGroup;
         QLabel* _pFromTimeLabel;
         QLabel* _pToTimeLabel;
         QTimeEdit* _pFromTime;
         QTimeEdit* _pToTime;
         QGroupBox* _pModeGroup;
         QComboBox* _pDisplayMode;
         QLabel* _pHeatScaleCaption;
         QSlider* _pHeatScaleSlider;
         QLabel* _pHeatScaleValueLabel;
         QGroupBox* _pInfoGroup;
         QLabel* _pWhenCaption;
         QLabel* _pLatitudeCaption;
         QLabel* _pLongitudeCaption;
         QGroupBox* _pLanguageGroup;
         QComboBox* _pLanguageCombo;
         QPushButton* _pZoomInButton;
         QPushButton* _pZoomOutButton;
         QSlider* _pZoomSlider;
         QLabel* _pTimeLabel;
         QLabel* _pLatitudeLabel;
         QLabel* _pLongitudeLabel;
   };
} // namespace LocationHistory

#endif // MAIN_WINDOW_H
