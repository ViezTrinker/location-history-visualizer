/*!
 *\file main_window.h
 *\brief Main application window with filters, map, Settings, and About menus
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <array>
#include <cstdint>

#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QTimeEdit>
#include <QTimer>
#include <QWidget>

#include "load_result.h"
#include "location_data.h"
#include "location_filter.h"
#include "location_point.h"
#include "map_widget.h"

namespace LocationHistory
{
   constexpr size_t WeekdayCount = 7;
   using WeekdayBoxList = std::array<QCheckBox*, WeekdayCount>;
   using WeekdayLabelList = std::array<QLabel*, WeekdayCount>;

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
         void OnZoomInClicked(void);
         void OnZoomOutClicked(void);
         void OnZoomSliderChanged(int zoomValue);
         void OnMapZoomChanged(int32_t zoom);
         void OnLanguageActionTriggered(QAction* pAction);
         void OnThemeActionTriggered(QAction* pAction);
         void OnPointRadiusChanged(int pointRadiusPx);
         void OnDrawnPointLimitChanged(int drawnPointLimit);
         void OnStoryDateChanged(const QDate& date);
         void OnStorySliderChanged(int sliderValue);
         void OnStoryPlayClicked(void);
         void OnStoryTimerTick(void);
         void OnPointClicked(
            double latitude,
            double longitude,
            int64_t unixTimeMs,
            int32_t utcOffsetMinutes,
            int64_t endUnixTimeMs,
            PointSource source);
         void OnPointCleared(void);

      protected:
         void changeEvent(QEvent* pEvent) override;

      private:
         enum class StoryPlayback : uint8_t
         {
            Stopped = 0,
            Playing = 1
         };

         void BuildUi(void);
         void BuildMenus(void);
         void RetranslateUi(void);
         void FillLanguageMenu(void);
         void FillThemeMenu(void);
         void UpdatePointCounts(void);
         void UpdateStatusMessage(void);
         void UpdateStoryControls(void);
         void UpdateStoryBarVisibility(void);
         void UpdateStoryDateLimits(void);
         void RefreshDisplayedPoints(void);
         void ApplyStoryCutoff(void);
         void StopStoryPlayback(void);
         DisplayMode CurrentDisplayMode(void) const;
         size_t MatchingPointCount(void) const;
         size_t VisiblePointCount(void) const;
         QString WeekdayText(size_t weekdayIndex) const;
         QString LoadResultMessage(LoadResult result) const;
         QString FormatDuration(int64_t durationMs) const;
         FilterSettings ReadFilterSettings(void) const;
         void ApplyCurrentFilter(void);
         void UpdateDateRangeFromPoints(void);
         void SyncZoomSlider(void);

         LocationPointList _allPoints;
         LocationPointList _filteredPoints;
         LocationPointList _storyDayPoints;
         QString _loadedFilePath;
         int64_t _storyMinTimeMs;
         int64_t _storyMaxTimeMs;
         StoryPlayback _storyPlayback;
         MapWidget* _pMapWidget;
         QMenu* _pFileMenu;
         QAction* _pOpenAction;
         QAction* _pQuitAction;
         QMenu* _pSettingsMenu;
         QMenu* _pLanguageMenu;
         QActionGroup* _pLanguageActions;
         QMenu* _pThemeMenu;
         QActionGroup* _pThemeActions;
         QAction* _pDarkThemeAction;
         QAction* _pLightThemeAction;
         QMenu* _pHelpMenu;
         QAction* _pAboutAction;
         QGroupBox* _pCountGroup;
         QLabel* _pInFileCaption;
         QLabel* _pVisibleCaption;
         QLabel* _pInFileCountLabel;
         QLabel* _pVisibleCountLabel;
         QGroupBox* _pDateGroup;
         QLabel* _pFromDateLabel;
         QLabel* _pToDateLabel;
         QDateEdit* _pFromDate;
         QDateEdit* _pToDate;
         QGroupBox* _pWeekdayGroup;
         WeekdayBoxList _pWeekdayBoxes;
         WeekdayLabelList _pWeekdayLabels;
         QGroupBox* _pTimeGroup;
         QLabel* _pFromTimeLabel;
         QLabel* _pToTimeLabel;
         QTimeEdit* _pFromTime;
         QTimeEdit* _pToTime;
         QGroupBox* _pModeGroup;
         QComboBox* _pDisplayMode;
         QGroupBox* _pInfoGroup;
         QLabel* _pWhenCaption;
         QLabel* _pUntilCaption;
         QLabel* _pDurationCaption;
         QLabel* _pLatitudeCaption;
         QLabel* _pLongitudeCaption;
         QGroupBox* _pMapDisplayGroup;
         QLabel* _pPointSizeCaption;
         QLabel* _pDrawnLimitCaption;
         QSpinBox* _pPointRadiusSpin;
         QSpinBox* _pDrawnPointLimitSpin;
         QPushButton* _pZoomInButton;
         QPushButton* _pZoomOutButton;
         QSlider* _pZoomSlider;
         QPushButton* _pStoryPlayButton;
         QDateEdit* _pStoryDate;
         QSlider* _pStorySlider;
         QLabel* _pStoryTimeLabel;
         QTimer* _pStoryTimer;
         QWidget* _pStoryBar;
         QLabel* _pTimeLabel;
         QLabel* _pUntilLabel;
         QLabel* _pDurationLabel;
         QLabel* _pLatitudeLabel;
         QLabel* _pLongitudeLabel;
   };
} // namespace LocationHistory

#endif // MAIN_WINDOW_H
