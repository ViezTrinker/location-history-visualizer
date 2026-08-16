/*!
 *\file main_window.cpp
 *\brief Main application window with filters, map, Settings, and About menus
 */

#include "main_window.h"

#include <cstdint>
#include <string>
#include <string_view>

#include <QAction>
#include <QActionGroup>
#include <QByteArray>
#include <QCloseEvent>
#include <QDate>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QThread>
#include <QTime>
#include <QVBoxLayout>
#include <QWidget>

#include "about_dialog.h"
#include "app_language.h"
#include "app_theme.h"
#include "civil_time.h"
#include "json_load_thread.h"
#include "load_result.h"
#include "location_filter.h"
#include "location_point.h"
#include "map_display_settings.h"
#include "map_widget.h"
#include "story_time.h"
#include "tile_math.h"
#include "version.h"

namespace LocationHistory
{
   namespace
   {
      inline constexpr int32_t ZoomButtonSizePx = 32;
      inline constexpr int32_t ZoomPanelWidthPx = 48;
      inline constexpr int32_t StoryTimerIntervalMs = 40;
      inline constexpr int32_t LoadProgressMax = 1000;
      inline constexpr int32_t DefaultWindowWidthPx = 1280;
      inline constexpr int32_t DefaultWindowHeightPx = 800;

      QString LastJsonSettingsKey(void)
      {
         return QStringLiteral("lastJsonPath");
      }

      QString WindowGeometrySettingsKey(void)
      {
         return QStringLiteral("windowGeometry");
      }

      QString LastJsonDialogPath(void)
      {
         QSettings settings;
         const QString storedPath = settings.value(LastJsonSettingsKey()).toString();
         if (storedPath.isEmpty())
         {
            return QString();
         }

         const QFileInfo fileInfo(storedPath);
         if (fileInfo.exists())
         {
            return storedPath;
         }

         const QString directory = fileInfo.absolutePath();
         if (QDir(directory).exists())
         {
            return directory;
         }

         return QString();
      }

      QString FormatCount(const size_t count)
      {
         return QLocale().toString(static_cast<qulonglong>(count));
      }
   } // namespace

   MainWindow::MainWindow(QWidget* pParent)
      : QMainWindow(pParent)
      , _storyMinTimeMs(0)
      , _storyMaxTimeMs(0)
      , _storyPlayback(StoryPlayback::Stopped)
      , _pMapWidget(nullptr)
      , _pFileMenu(nullptr)
      , _pOpenAction(nullptr)
      , _pQuitAction(nullptr)
      , _pSettingsMenu(nullptr)
      , _pLanguageMenu(nullptr)
      , _pLanguageActions(nullptr)
      , _pThemeMenu(nullptr)
      , _pThemeActions(nullptr)
      , _pThemeItemActions{}
      , _pHelpMenu(nullptr)
      , _pAboutAction(nullptr)
      , _pCountGroup(nullptr)
      , _pInFileCaption(nullptr)
      , _pVisibleCaption(nullptr)
      , _pInFileCountLabel(nullptr)
      , _pVisibleCountLabel(nullptr)
      , _pDateGroup(nullptr)
      , _pFromDateLabel(nullptr)
      , _pToDateLabel(nullptr)
      , _pFromDate(nullptr)
      , _pToDate(nullptr)
      , _pWeekdayGroup(nullptr)
      , _pWeekdayBoxes{}
      , _pWeekdayLabels{}
      , _pTimeGroup(nullptr)
      , _pFromTimeLabel(nullptr)
      , _pToTimeLabel(nullptr)
      , _pFromTime(nullptr)
      , _pToTime(nullptr)
      , _pModeGroup(nullptr)
      , _pDisplayMode(nullptr)
      , _pInfoGroup(nullptr)
      , _pWhenCaption(nullptr)
      , _pUntilCaption(nullptr)
      , _pDurationCaption(nullptr)
      , _pLatitudeCaption(nullptr)
      , _pLongitudeCaption(nullptr)
      , _pMapDisplayGroup(nullptr)
      , _pPointSizeCaption(nullptr)
      , _pDrawnLimitCaption(nullptr)
      , _pPointRadiusSpin(nullptr)
      , _pDrawnPointLimitSpin(nullptr)
      , _pZoomInButton(nullptr)
      , _pZoomOutButton(nullptr)
      , _pZoomSlider(nullptr)
      , _pStoryPlayButton(nullptr)
      , _pStoryDate(nullptr)
      , _pStorySlider(nullptr)
      , _pStoryTimeLabel(nullptr)
      , _pStoryTimer(nullptr)
      , _pStoryBar(nullptr)
      , _pTimeLabel(nullptr)
      , _pUntilLabel(nullptr)
      , _pDurationLabel(nullptr)
      , _pLatitudeLabel(nullptr)
      , _pLongitudeLabel(nullptr)
      , _pLoadDialog(nullptr)
      , _pLoadThread(nullptr)
   {
      setWindowTitle(QString::fromUtf8(AppName.data(), static_cast<int>(AppName.size())));
      RestoreWindowGeometry();
      BuildMenus();
      BuildUi();
      RetranslateUi();
      OnPointCleared();
   }

   MainWindow::~MainWindow(void)
   {
      if (_pLoadThread == nullptr)
      {
         return;
      }

      disconnect(_pLoadThread, nullptr, this, nullptr);
      _pLoadThread->RequestCancel();
      _pLoadThread->wait();
      delete _pLoadThread;
      _pLoadThread = nullptr;
   }

   void MainWindow::BuildMenus(void)
   {
      _pFileMenu = menuBar()->addMenu(QString());
      _pOpenAction = _pFileMenu->addAction(QString());
      _pOpenAction->setShortcut(QKeySequence::Open);
      connect(_pOpenAction, &QAction::triggered, this, &MainWindow::OnOpenClicked);

      _pQuitAction = _pFileMenu->addAction(QString());
      _pQuitAction->setShortcut(QKeySequence::Quit);
      connect(_pQuitAction, &QAction::triggered, this, &MainWindow::close);

      _pSettingsMenu = menuBar()->addMenu(QString());
      FillLanguageMenu();
      FillThemeMenu();

      _pHelpMenu = menuBar()->addMenu(QString());
      _pAboutAction = _pHelpMenu->addAction(QString());
      connect(_pAboutAction, &QAction::triggered, this, &MainWindow::OnAbout);
   }

   void MainWindow::BuildUi(void)
   {
      QWidget* pCentral = new QWidget(this);
      setCentralWidget(pCentral);

      QWidget* pSidePanel = new QWidget(pCentral);
      pSidePanel->setFixedWidth(280);
      QVBoxLayout* pSideLayout = new QVBoxLayout(pSidePanel);

      _pDateGroup = new QGroupBox(pSidePanel);
      QFormLayout* pDateLayout = new QFormLayout(_pDateGroup);
      _pFromDate = new QDateEdit(QDate(1970, 1, 1), _pDateGroup);
      _pToDate = new QDateEdit(QDate(9999, 12, 31), _pDateGroup);
      _pFromDateLabel = new QLabel(_pDateGroup);
      _pToDateLabel = new QLabel(_pDateGroup);
      _pFromDate->setCalendarPopup(true);
      _pToDate->setCalendarPopup(true);
      _pFromDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
      _pToDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
      connect(_pFromDate, &QDateEdit::dateChanged, this, &MainWindow::OnFiltersChanged);
      connect(_pToDate, &QDateEdit::dateChanged, this, &MainWindow::OnFiltersChanged);
      pDateLayout->addRow(_pFromDateLabel, _pFromDate);
      pDateLayout->addRow(_pToDateLabel, _pToDate);
      pSideLayout->addWidget(_pDateGroup);

      _pWeekdayGroup = new QGroupBox(pSidePanel);
      QHBoxLayout* pWeekdayLayout = new QHBoxLayout(_pWeekdayGroup);
      pWeekdayLayout->setSpacing(0);
      for (size_t weekdayIndex = 0; weekdayIndex < WeekdayCount; ++weekdayIndex)
      {
         QWidget* pColumn = new QWidget(_pWeekdayGroup);
         QVBoxLayout* pColumnLayout = new QVBoxLayout(pColumn);
         pColumnLayout->setContentsMargins(0, 0, 0, 0);
         pColumnLayout->setSpacing(2);

         QCheckBox* pBox = new QCheckBox(pColumn);
         pBox->setChecked(true);
         pBox->setText(QString());
         connect(pBox, &QCheckBox::stateChanged, this, &MainWindow::OnFiltersChanged);

         QLabel* pLabel = new QLabel(pColumn);
         pLabel->setAlignment(Qt::AlignHCenter);
         pLabel->setWordWrap(true);

         pColumnLayout->addWidget(pBox, 0, Qt::AlignHCenter);
         pColumnLayout->addWidget(pLabel);
         _pWeekdayBoxes[weekdayIndex] = pBox;
         _pWeekdayLabels[weekdayIndex] = pLabel;
         pWeekdayLayout->addWidget(pColumn, 1);
      }
      pSideLayout->addWidget(_pWeekdayGroup);

      _pTimeGroup = new QGroupBox(pSidePanel);
      QFormLayout* pTimeLayout = new QFormLayout(_pTimeGroup);
      _pFromTime = new QTimeEdit(QTime(0, 0), _pTimeGroup);
      _pToTime = new QTimeEdit(QTime(23, 59), _pTimeGroup);
      _pFromTimeLabel = new QLabel(_pTimeGroup);
      _pToTimeLabel = new QLabel(_pTimeGroup);
      _pFromTime->setDisplayFormat(QStringLiteral("HH:mm"));
      _pToTime->setDisplayFormat(QStringLiteral("HH:mm"));
      connect(_pFromTime, &QTimeEdit::timeChanged, this, &MainWindow::OnFiltersChanged);
      connect(_pToTime, &QTimeEdit::timeChanged, this, &MainWindow::OnFiltersChanged);
      pTimeLayout->addRow(_pFromTimeLabel, _pFromTime);
      pTimeLayout->addRow(_pToTimeLabel, _pToTime);
      pSideLayout->addWidget(_pTimeGroup);

      _pModeGroup = new QGroupBox(pSidePanel);
      QVBoxLayout* pModeLayout = new QVBoxLayout(_pModeGroup);
      _pDisplayMode = new QComboBox(_pModeGroup);
      _pDisplayMode->addItem(QString(), static_cast<int>(DisplayMode::Points));
      _pDisplayMode->addItem(QString(), static_cast<int>(DisplayMode::Clustered));
      _pDisplayMode->addItem(QString(), static_cast<int>(DisplayMode::Story));
      connect(_pDisplayMode, &QComboBox::currentIndexChanged, this, &MainWindow::OnDisplayModeChanged);
      pModeLayout->addWidget(_pDisplayMode);
      pSideLayout->addWidget(_pModeGroup);

      _pInfoGroup = new QGroupBox(pSidePanel);
      QFormLayout* pInfoLayout = new QFormLayout(_pInfoGroup);
      _pWhenCaption = new QLabel(_pInfoGroup);
      _pUntilCaption = new QLabel(_pInfoGroup);
      _pDurationCaption = new QLabel(_pInfoGroup);
      _pLatitudeCaption = new QLabel(_pInfoGroup);
      _pLongitudeCaption = new QLabel(_pInfoGroup);
      _pTimeLabel = new QLabel(QStringLiteral("—"), _pInfoGroup);
      _pUntilLabel = new QLabel(QStringLiteral("—"), _pInfoGroup);
      _pDurationLabel = new QLabel(QStringLiteral("—"), _pInfoGroup);
      _pLatitudeLabel = new QLabel(QStringLiteral("—"), _pInfoGroup);
      _pLongitudeLabel = new QLabel(QStringLiteral("—"), _pInfoGroup);
      _pTimeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      _pUntilLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      _pDurationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      _pLatitudeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      _pLongitudeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      pInfoLayout->addRow(_pWhenCaption, _pTimeLabel);
      pInfoLayout->addRow(_pUntilCaption, _pUntilLabel);
      pInfoLayout->addRow(_pDurationCaption, _pDurationLabel);
      pInfoLayout->addRow(_pLatitudeCaption, _pLatitudeLabel);
      pInfoLayout->addRow(_pLongitudeCaption, _pLongitudeLabel);
      pSideLayout->addWidget(_pInfoGroup);

      _pMapDisplayGroup = new QGroupBox(pSidePanel);
      QFormLayout* pMapDisplayLayout = new QFormLayout(_pMapDisplayGroup);
      _pPointSizeCaption = new QLabel(_pMapDisplayGroup);
      _pDrawnLimitCaption = new QLabel(_pMapDisplayGroup);
      _pPointRadiusSpin = new QSpinBox(_pMapDisplayGroup);
      _pPointRadiusSpin->setRange(MinPointRadiusPx, MaxPointRadiusPx);
      _pPointRadiusSpin->setSingleStep(1);
      _pPointRadiusSpin->setValue(LoadPointRadiusPx());
      _pDrawnPointLimitSpin = new QSpinBox(_pMapDisplayGroup);
      _pDrawnPointLimitSpin->setRange(MinDrawnPointLimit, MaxDrawnPointLimit);
      _pDrawnPointLimitSpin->setSingleStep(DrawnPointLimitSpinStep);
      _pDrawnPointLimitSpin->setAccelerated(true);
      _pDrawnPointLimitSpin->setGroupSeparatorShown(true);
      _pDrawnPointLimitSpin->setValue(LoadDrawnPointLimit());
      connect(_pPointRadiusSpin, &QSpinBox::valueChanged, this, &MainWindow::OnPointRadiusChanged);
      connect(_pDrawnPointLimitSpin, &QSpinBox::valueChanged, this, &MainWindow::OnDrawnPointLimitChanged);
      pMapDisplayLayout->addRow(_pPointSizeCaption, _pPointRadiusSpin);
      pMapDisplayLayout->addRow(_pDrawnLimitCaption, _pDrawnPointLimitSpin);
      pSideLayout->addWidget(_pMapDisplayGroup);
      pSideLayout->addStretch();

      _pCountGroup = new QGroupBox(pSidePanel);
      QFormLayout* pCountLayout = new QFormLayout(_pCountGroup);
      _pInFileCaption = new QLabel(_pCountGroup);
      _pVisibleCaption = new QLabel(_pCountGroup);
      _pInFileCountLabel = new QLabel(QStringLiteral("0"), _pCountGroup);
      _pVisibleCountLabel = new QLabel(QStringLiteral("0"), _pCountGroup);
      _pInFileCountLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      _pVisibleCountLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      pCountLayout->addRow(_pInFileCaption, _pInFileCountLabel);
      pCountLayout->addRow(_pVisibleCaption, _pVisibleCountLabel);
      pSideLayout->addWidget(_pCountGroup);

      _pMapWidget = new MapWidget(pCentral);
      const auto pointRadiusPx = static_cast<int32_t>(_pPointRadiusSpin->value());
      const auto drawnPointLimit = static_cast<int32_t>(_pDrawnPointLimitSpin->value());
      _pMapWidget->SetPointRadiusPx(pointRadiusPx);
      _pMapWidget->SetDrawnPointLimit(drawnPointLimit);
      connect(_pMapWidget, &MapWidget::PointClicked, this, &MainWindow::OnPointClicked);
      connect(_pMapWidget, &MapWidget::PointCleared, this, &MainWindow::OnPointCleared);
      connect(_pMapWidget, &MapWidget::ZoomChanged, this, &MainWindow::OnMapZoomChanged);

      QWidget* pZoomPanel = new QWidget(pCentral);
      pZoomPanel->setFixedWidth(ZoomPanelWidthPx);
      _pZoomInButton = new QPushButton(QStringLiteral("+"), pZoomPanel);
      _pZoomOutButton = new QPushButton(QStringLiteral("-"), pZoomPanel);
      _pZoomSlider = new QSlider(Qt::Vertical, pZoomPanel);
      _pZoomInButton->setFixedSize(ZoomButtonSizePx, ZoomButtonSizePx);
      _pZoomOutButton->setFixedSize(ZoomButtonSizePx, ZoomButtonSizePx);
      _pZoomSlider->setRange(MinZoom, MaxZoom);
      SyncZoomSlider();
      connect(_pZoomInButton, &QPushButton::clicked, this, &MainWindow::OnZoomInClicked);
      connect(_pZoomOutButton, &QPushButton::clicked, this, &MainWindow::OnZoomOutClicked);
      connect(_pZoomSlider, &QSlider::valueChanged, this, &MainWindow::OnZoomSliderChanged);

      QVBoxLayout* pZoomLayout = new QVBoxLayout(pZoomPanel);
      pZoomLayout->setContentsMargins(4, 4, 4, 4);
      pZoomLayout->setSpacing(4);
      pZoomLayout->addWidget(_pZoomInButton, 0, Qt::AlignHCenter);
      pZoomLayout->addWidget(_pZoomSlider, 1, Qt::AlignHCenter);
      pZoomLayout->addWidget(_pZoomOutButton, 0, Qt::AlignHCenter);

      _pStoryPlayButton = new QPushButton(pCentral);
      _pStoryDate = new QDateEdit(QDate(1970, 1, 1), pCentral);
      _pStorySlider = new QSlider(Qt::Horizontal, pCentral);
      _pStoryTimeLabel = new QLabel(pCentral);
      _pStoryDate->setCalendarPopup(true);
      _pStoryDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
      _pStorySlider->setRange(0, StorySliderMax);
      _pStorySlider->setValue(0);
      _pStoryTimeLabel->setMinimumWidth(180);
      _pStoryTimer = new QTimer(this);
      _pStoryTimer->setInterval(StoryTimerIntervalMs);
      connect(_pStoryPlayButton, &QPushButton::clicked, this, &MainWindow::OnStoryPlayClicked);
      connect(_pStoryDate, &QDateEdit::dateChanged, this, &MainWindow::OnStoryDateChanged);
      connect(_pStorySlider, &QSlider::valueChanged, this, &MainWindow::OnStorySliderChanged);
      connect(_pStoryTimer, &QTimer::timeout, this, &MainWindow::OnStoryTimerTick);

      _pStoryBar = new QWidget(pCentral);
      QHBoxLayout* pStoryLayout = new QHBoxLayout(_pStoryBar);
      pStoryLayout->setContentsMargins(0, 0, 0, 0);
      pStoryLayout->addWidget(_pStoryDate);
      pStoryLayout->addWidget(_pStoryPlayButton);
      pStoryLayout->addWidget(_pStorySlider, 1);
      pStoryLayout->addWidget(_pStoryTimeLabel);

      QWidget* pMapColumn = new QWidget(pCentral);
      QVBoxLayout* pMapColumnLayout = new QVBoxLayout(pMapColumn);
      pMapColumnLayout->setContentsMargins(0, 0, 0, 0);
      QHBoxLayout* pMapRow = new QHBoxLayout();
      pMapRow->setContentsMargins(0, 0, 0, 0);
      pMapRow->addWidget(_pMapWidget, 1);
      pMapRow->addWidget(pZoomPanel);
      pMapColumnLayout->addLayout(pMapRow, 1);
      pMapColumnLayout->addWidget(_pStoryBar);

      QHBoxLayout* pRootLayout = new QHBoxLayout(pCentral);
      pRootLayout->addWidget(pSidePanel);
      pRootLayout->addWidget(pMapColumn, 1);

      UpdateStoryBarVisibility();
      RefreshDisplayedPoints();
   }

   void MainWindow::FillLanguageMenu(void)
   {
      if (_pSettingsMenu == nullptr)
      {
         return;
      }

      _pLanguageMenu = _pSettingsMenu->addMenu(QString());
      _pLanguageActions = new QActionGroup(this);
      _pLanguageActions->setExclusive(true);
      connect(_pLanguageActions, &QActionGroup::triggered, this, &MainWindow::OnLanguageActionTriggered);

      const AppLanguage currentLanguage = LoadLanguageSetting();
      for (uint8_t languageValue = 0; languageValue < AppLanguageCount; ++languageValue)
      {
         const auto language = static_cast<AppLanguage>(languageValue);
         const std::string_view nativeName = LanguageNativeName(language);
         QAction* pAction = _pLanguageMenu->addAction(
            QString::fromUtf8(nativeName.data(), static_cast<int>(nativeName.size())));
         pAction->setCheckable(true);
         pAction->setData(static_cast<int>(languageValue));
         _pLanguageActions->addAction(pAction);
         if (language == currentLanguage)
         {
            pAction->setChecked(true);
         }
      }
   }

   void MainWindow::FillThemeMenu(void)
   {
      if (_pSettingsMenu == nullptr)
      {
         return;
      }

      _pThemeMenu = _pSettingsMenu->addMenu(QString());
      _pThemeActions = new QActionGroup(this);
      _pThemeActions->setExclusive(true);
      connect(_pThemeActions, &QActionGroup::triggered, this, &MainWindow::OnThemeActionTriggered);

      const AppTheme currentTheme = LoadThemeSetting();
      for (uint8_t themeValue = 0; themeValue < AppThemeCount; ++themeValue)
      {
         const auto theme = static_cast<AppTheme>(themeValue);
         QAction* pAction = _pThemeMenu->addAction(QString());
         pAction->setCheckable(true);
         pAction->setData(static_cast<int>(themeValue));
         _pThemeActions->addAction(pAction);
         _pThemeItemActions[themeValue] = pAction;
         if (theme == currentTheme)
         {
            pAction->setChecked(true);
         }
      }
   }

   void MainWindow::RetranslateUi(void)
   {
      _pFileMenu->setTitle(tr("&File"));
      _pOpenAction->setText(tr("Open..."));
      _pQuitAction->setText(tr("E&xit"));
      _pSettingsMenu->setTitle(tr("&Settings"));
      _pLanguageMenu->setTitle(tr("Language"));
      _pThemeMenu->setTitle(tr("Theme"));
      for (uint8_t themeValue = 0; themeValue < AppThemeCount; ++themeValue)
      {
         const auto theme = static_cast<AppTheme>(themeValue);
         _pThemeItemActions[themeValue]->setText(ThemeText(theme));
      }
      _pHelpMenu->setTitle(tr("&Help"));
      _pAboutAction->setText(tr("About"));
      _pCountGroup->setTitle(tr("Counts"));
      _pInFileCaption->setText(tr("In file"));
      _pVisibleCaption->setText(tr("Visible"));
      _pDateGroup->setTitle(tr("Date"));
      _pFromDateLabel->setText(tr("From"));
      _pToDateLabel->setText(tr("To"));
      _pWeekdayGroup->setTitle(tr("Weekday"));
      for (size_t weekdayIndex = 0; weekdayIndex < WeekdayCount; ++weekdayIndex)
      {
         _pWeekdayLabels[weekdayIndex]->setText(WeekdayText(weekdayIndex));
      }
      _pTimeGroup->setTitle(tr("Time of day"));
      _pFromTimeLabel->setText(tr("From"));
      _pToTimeLabel->setText(tr("To"));
      _pModeGroup->setTitle(tr("Display"));
      _pDisplayMode->setItemText(0, tr("Points"));
      _pDisplayMode->setItemText(1, tr("Cluster"));
      _pDisplayMode->setItemText(2, tr("Story"));
      _pInfoGroup->setTitle(tr("Point info"));
      _pWhenCaption->setText(tr("When"));
      _pUntilCaption->setText(tr("Until"));
      _pDurationCaption->setText(tr("Duration"));
      _pLatitudeCaption->setText(tr("Latitude"));
      _pLongitudeCaption->setText(tr("Longitude"));
      _pMapDisplayGroup->setTitle(tr("Map display"));
      _pPointSizeCaption->setText(tr("Point size"));
      _pDrawnLimitCaption->setText(tr("Maximum drawn points"));
      _pPointRadiusSpin->setSuffix(tr(" px"));
      if (_storyPlayback == StoryPlayback::Playing)
      {
         _pStoryPlayButton->setText(tr("Pause"));
      }
      else
      {
         _pStoryPlayButton->setText(tr("Play"));
      }
      _pStorySlider->setToolTip(tr("Reveal locations up to this time"));
      _pStoryDate->setToolTip(tr("Story start day"));
      _pZoomInButton->setToolTip(tr("Zoom in"));
      _pZoomOutButton->setToolTip(tr("Zoom out"));
      _pZoomSlider->setToolTip(tr("Zoom"));
      ApplyStoryCutoff();
      UpdateStatusMessage();
      UpdateLoadDialogText();
   }

   void MainWindow::UpdatePointCounts(void)
   {
      if (_pInFileCountLabel == nullptr)
      {
         return;
      }
      if (_pVisibleCountLabel == nullptr)
      {
         return;
      }

      _pInFileCountLabel->setText(FormatCount(_allPoints.size()));
      _pVisibleCountLabel->setText(FormatCount(VisiblePointCount()));
   }

   void MainWindow::UpdateStatusMessage(void)
   {
      UpdatePointCounts();
      if (_loadedFilePath.isEmpty())
      {
         statusBar()->showMessage(tr("Ready"));
         return;
      }

      const size_t matchingCount = MatchingPointCount();
      const size_t visibleCount = VisiblePointCount();
      if (visibleCount < matchingCount)
      {
         statusBar()->showMessage(tr("%1 of %2 points shown").arg(visibleCount).arg(matchingCount));
         return;
      }
      statusBar()->showMessage(tr("%1 points shown").arg(visibleCount));
   }

   size_t MainWindow::MatchingPointCount(void) const
   {
      if (CurrentDisplayMode() == DisplayMode::Story)
      {
         return _storyDayPoints.size();
      }
      return _filteredPoints.size();
   }

   size_t MainWindow::VisiblePointCount(void) const
   {
      const size_t matchingCount = MatchingPointCount();
      if (CurrentDisplayMode() == DisplayMode::Clustered)
      {
         return matchingCount;
      }
      if (_pMapWidget == nullptr)
      {
         return matchingCount;
      }
      return DrawnPointCount(matchingCount, _pMapWidget->DrawnPointLimit());
   }

   QString MainWindow::WeekdayText(const size_t weekdayIndex) const
   {
      switch (weekdayIndex)
      {
         case 0:
            return tr("Mon");
         case 1:
            return tr("Tue");
         case 2:
            return tr("Wed");
         case 3:
            return tr("Thu");
         case 4:
            return tr("Fri");
         case 5:
            return tr("Sat");
         default:
            return tr("Sun");
      }
   }

   QString MainWindow::ThemeText(const AppTheme theme) const
   {
      if (theme == AppTheme::Light)
      {
         return tr("Light");
      }
      if (theme == AppTheme::Midnight)
      {
         return tr("Midnight");
      }
      if (theme == AppTheme::Nord)
      {
         return tr("Nord");
      }
      if (theme == AppTheme::Sepia)
      {
         return tr("Sepia");
      }
      return tr("Dark");
   }

   QString MainWindow::LoadResultMessage(const LoadResult result) const
   {
      if (result == LoadResult::FileNotFound)
      {
         return tr("File not found.");
      }
      if (result == LoadResult::InvalidJson)
      {
         return tr("The JSON file is invalid.");
      }
      if (result == LoadResult::NoPoints)
      {
         return tr("The file contains no location data.");
      }
      return tr("The file could not be loaded.");
   }

   void MainWindow::changeEvent(QEvent* pEvent)
   {
      if (pEvent->type() == QEvent::LanguageChange)
      {
         RetranslateUi();
      }
      QMainWindow::changeEvent(pEvent);
   }

   void MainWindow::closeEvent(QCloseEvent* pEvent)
   {
      SaveWindowGeometry();
      QMainWindow::closeEvent(pEvent);
   }

   void MainWindow::RestoreWindowGeometry(void)
   {
      QSettings settings;
      if (!settings.contains(WindowGeometrySettingsKey()))
      {
         resize(DefaultWindowWidthPx, DefaultWindowHeightPx);
         return;
      }

      const QByteArray geometry = settings.value(WindowGeometrySettingsKey()).toByteArray();
      if (geometry.isEmpty())
      {
         resize(DefaultWindowWidthPx, DefaultWindowHeightPx);
         return;
      }
      if (!restoreGeometry(geometry))
      {
         resize(DefaultWindowWidthPx, DefaultWindowHeightPx);
      }
   }

   void MainWindow::SaveWindowGeometry(void)
   {
      QSettings settings;
      settings.setValue(WindowGeometrySettingsKey(), saveGeometry());
   }

   FilterSettings MainWindow::ReadFilterSettings(void) const
   {
      FilterSettings settings = MakePassThroughFilter();
      settings.dateFilter = FilterActive::Yes;
      settings.fromYear = _pFromDate->date().year();
      settings.fromMonth = _pFromDate->date().month();
      settings.fromDay = _pFromDate->date().day();
      settings.toYear = _pToDate->date().year();
      settings.toMonth = _pToDate->date().month();
      settings.toDay = _pToDate->date().day();

      uint8_t weekdayMask = 0;
      for (size_t weekdayIndex = 0; weekdayIndex < WeekdayCount; ++weekdayIndex)
      {
         if (_pWeekdayBoxes[weekdayIndex]->isChecked())
         {
            weekdayMask = static_cast<uint8_t>(weekdayMask | (1u << weekdayIndex));
         }
      }
      settings.weekdayMask = weekdayMask;

      settings.timeFilter = FilterActive::Yes;
      settings.fromMinuteOfDay = _pFromTime->time().hour() * 60 + _pFromTime->time().minute();
      settings.toMinuteOfDay = _pToTime->time().hour() * 60 + _pToTime->time().minute();
      return settings;
   }

   void MainWindow::ApplyCurrentFilter(void)
   {
      const FilterSettings settings = ReadFilterSettings();
      ApplyFilter(_allPoints, settings, _filteredPoints);
      StopStoryPlayback();
      UpdateStoryDateLimits();
      RefreshDisplayedPoints();
      UpdateStatusMessage();
   }

   void MainWindow::UpdateDateRangeFromPoints(void)
   {
      if (_allPoints.empty())
      {
         return;
      }

      CivilDateTime firstTime{};
      UnixMsToCivil(_allPoints[0].unixTimeMs, _allPoints[0].utcOffsetMinutes, firstTime);
      int32_t minPacked = firstTime.year * 10000 + firstTime.month * 100 + firstTime.day;
      int32_t maxPacked = minPacked;
      CivilDateTime minDate = firstTime;
      CivilDateTime maxDate = firstTime;

      for (size_t index = 1; index < _allPoints.size(); ++index)
      {
         CivilDateTime dateTime{};
         UnixMsToCivil(_allPoints[index].unixTimeMs, _allPoints[index].utcOffsetMinutes, dateTime);
         const int32_t packed = dateTime.year * 10000 + dateTime.month * 100 + dateTime.day;
         if (packed < minPacked)
         {
            minPacked = packed;
            minDate = dateTime;
         }
         if (packed > maxPacked)
         {
            maxPacked = packed;
            maxDate = dateTime;
         }
      }

      _pFromDate->blockSignals(true);
      _pToDate->blockSignals(true);
      _pFromDate->setDate(QDate(minDate.year, minDate.month, minDate.day));
      _pToDate->setDate(QDate(maxDate.year, maxDate.month, maxDate.day));
      _pFromDate->blockSignals(false);
      _pToDate->blockSignals(false);
   }

   void MainWindow::OnOpenClicked(void)
   {
      if (_pLoadThread != nullptr)
      {
         return;
      }

      const QString path = QFileDialog::getOpenFileName(
         this,
         tr("Open Timeline JSON"),
         LastJsonDialogPath(),
         tr("JSON files (*.json);;All files (*.*)"));
      if (path.isEmpty())
      {
         return;
      }

      BeginFileLoad(path);
   }

   void MainWindow::BeginFileLoad(const QString& path)
   {
      if (_pLoadThread != nullptr)
      {
         return;
      }

      _pLoadDialog = new QProgressDialog(this);
      _pLoadDialog->setWindowModality(Qt::WindowModal);
      _pLoadDialog->setMinimumDuration(0);
      _pLoadDialog->setAutoClose(false);
      _pLoadDialog->setAutoReset(false);
      _pLoadDialog->setRange(0, LoadProgressMax);
      _pLoadDialog->setValue(0);
      UpdateLoadDialogText();
      connect(_pLoadDialog, &QProgressDialog::canceled, this, &MainWindow::OnLoadCancelClicked);

      _pLoadThread = new JsonLoadThread(path);
      connect(_pLoadThread, &JsonLoadThread::Progress, this, &MainWindow::OnLoadProgress);
      connect(_pLoadThread, &QThread::finished, this, &MainWindow::OnLoadThreadFinished);
      if (_pOpenAction != nullptr)
      {
         _pOpenAction->setEnabled(false);
      }
      _pLoadThread->start();
   }

   void MainWindow::UpdateLoadDialogText(void)
   {
      if (_pLoadDialog == nullptr)
      {
         return;
      }

      _pLoadDialog->setWindowTitle(tr("Loading"));
      _pLoadDialog->setLabelText(tr("Loading Timeline JSON…"));
      _pLoadDialog->setCancelButtonText(tr("Cancel"));
   }

   void MainWindow::CloseLoadDialog(void)
   {
      if (_pLoadDialog == nullptr)
      {
         return;
      }

      _pLoadDialog->disconnect(this);
      _pLoadDialog->close();
      _pLoadDialog->deleteLater();
      _pLoadDialog = nullptr;
   }

   void MainWindow::OnLoadProgress(const qint64 bytesRead, const qint64 bytesTotal)
   {
      if (_pLoadDialog == nullptr)
      {
         return;
      }
      if (bytesTotal <= 0)
      {
         _pLoadDialog->setRange(0, 0);
         return;
      }

      _pLoadDialog->setRange(0, LoadProgressMax);
      const auto value = static_cast<int>((bytesRead * static_cast<qint64>(LoadProgressMax)) / bytesTotal);
      _pLoadDialog->setValue(value);
   }

   void MainWindow::OnLoadCancelClicked(void)
   {
      if (_pLoadThread != nullptr)
      {
         _pLoadThread->RequestCancel();
      }
      if (_pLoadDialog != nullptr)
      {
         _pLoadDialog->setLabelText(tr("Cancelling…"));
      }
   }

   void MainWindow::OnLoadThreadFinished(void)
   {
      JsonLoadThread* pThread = _pLoadThread;
      _pLoadThread = nullptr;
      if (pThread == nullptr)
      {
         CloseLoadDialog();
         if (_pOpenAction != nullptr)
         {
            _pOpenAction->setEnabled(true);
         }
         return;
      }

      const LoadResult result = pThread->Result();
      const QString path = pThread->Path();
      LocationPointList loadedPoints;
      pThread->TakePoints(loadedPoints);
      pThread->deleteLater();

      CloseLoadDialog();
      if (_pOpenAction != nullptr)
      {
         _pOpenAction->setEnabled(true);
      }

      if (result == LoadResult::Cancelled)
      {
         return;
      }
      if (IsErr(result))
      {
         QMessageBox::warning(this, tr("Load failed"), LoadResultMessage(result));
         return;
      }

      _allPoints.swap(loadedPoints);
      QSettings settings;
      settings.setValue(LastJsonSettingsKey(), path);
      _loadedFilePath = path;
      UpdateDateRangeFromPoints();
      ApplyCurrentFilter();
      _pMapWidget->CenterOnPoints();
      OnPointCleared();
   }

   void MainWindow::OnAbout(void)
   {
      AboutDialog dialog(this);
      dialog.exec();
   }

   void MainWindow::OnPointRadiusChanged(const int pointRadiusPx)
   {
      if (_pMapWidget == nullptr)
      {
         return;
      }

      const auto radius = static_cast<int32_t>(pointRadiusPx);
      SavePointRadiusPx(radius);
      _pMapWidget->SetPointRadiusPx(radius);
   }

   void MainWindow::OnDrawnPointLimitChanged(const int drawnPointLimit)
   {
      if (_pMapWidget == nullptr)
      {
         return;
      }

      const auto limit = static_cast<int32_t>(drawnPointLimit);
      SaveDrawnPointLimit(limit);
      _pMapWidget->SetDrawnPointLimit(limit);
      UpdateStatusMessage();
   }

   void MainWindow::OnFiltersChanged(void)
   {
      ApplyCurrentFilter();
   }

   void MainWindow::OnDisplayModeChanged(const int index)
   {
      (void)index;
      StopStoryPlayback();
      _pMapWidget->SetDisplayMode(CurrentDisplayMode());
      UpdateStoryBarVisibility();
      RefreshDisplayedPoints();
      UpdateStatusMessage();
   }

   DisplayMode MainWindow::CurrentDisplayMode(void) const
   {
      if (_pDisplayMode == nullptr)
      {
         return DisplayMode::Points;
      }
      return static_cast<DisplayMode>(_pDisplayMode->currentData().toInt());
   }

   void MainWindow::UpdateStoryBarVisibility(void)
   {
      if (_pStoryBar == nullptr)
      {
         return;
      }
      if (CurrentDisplayMode() == DisplayMode::Story)
      {
         _pStoryBar->show();
         return;
      }
      _pStoryBar->hide();
   }

   void MainWindow::UpdateStoryDateLimits(void)
   {
      if (_pStoryDate == nullptr)
      {
         return;
      }
      if (_filteredPoints.empty())
      {
         _pStoryDate->setEnabled(false);
         return;
      }

      CivilDateTime firstTime{};
      UnixMsToCivil(_filteredPoints[0].unixTimeMs, _filteredPoints[0].utcOffsetMinutes, firstTime);
      int32_t minPacked = firstTime.year * 10000 + firstTime.month * 100 + firstTime.day;
      int32_t maxPacked = minPacked;
      CivilDateTime minDate = firstTime;
      CivilDateTime maxDate = firstTime;
      for (size_t index = 1; index < _filteredPoints.size(); ++index)
      {
         CivilDateTime dateTime{};
         UnixMsToCivil(_filteredPoints[index].unixTimeMs, _filteredPoints[index].utcOffsetMinutes, dateTime);
         const int32_t packed = dateTime.year * 10000 + dateTime.month * 100 + dateTime.day;
         if (packed < minPacked)
         {
            minPacked = packed;
            minDate = dateTime;
         }
         if (packed > maxPacked)
         {
            maxPacked = packed;
            maxDate = dateTime;
         }
      }

      const QDate rangeStart(minDate.year, minDate.month, minDate.day);
      const QDate rangeEnd(maxDate.year, maxDate.month, maxDate.day);
      _pStoryDate->setEnabled(true);
      _pStoryDate->blockSignals(true);
      _pStoryDate->setDateRange(rangeStart, rangeEnd);
      const QDate selected = _pStoryDate->date();
      if ((selected < rangeStart) || (selected > rangeEnd))
      {
         _pStoryDate->setDate(rangeStart);
      }
      _pStoryDate->blockSignals(false);
   }

   void MainWindow::RefreshDisplayedPoints(void)
   {
      if (_pMapWidget == nullptr)
      {
         return;
      }

      if (CurrentDisplayMode() != DisplayMode::Story)
      {
         _pMapWidget->SetUntilTime(ShowAllUntilTimeMs);
         _pMapWidget->SetPoints(_filteredPoints);
         return;
      }

      const QDate storyDate = _pStoryDate->date();
      CollectPointsFromDate(
         _filteredPoints,
         storyDate.year(),
         storyDate.month(),
         storyDate.day(),
         _storyDayPoints);
      _pMapWidget->SetPoints(_storyDayPoints);
      UpdateStoryControls();
   }

   void MainWindow::UpdateStoryControls(void)
   {
      if (_storyDayPoints.empty())
      {
         _storyMinTimeMs = 0;
         _storyMaxTimeMs = 0;
         _pStorySlider->setEnabled(false);
         _pStoryPlayButton->setEnabled(false);
         _pStoryTimeLabel->setText(QStringLiteral("—"));
         _pMapWidget->SetUntilTime(ShowAllUntilTimeMs);
         return;
      }

      _storyMinTimeMs = _storyDayPoints[0].unixTimeMs;
      _storyMaxTimeMs = _storyDayPoints[0].endUnixTimeMs;
      if (_storyMaxTimeMs < _storyMinTimeMs)
      {
         _storyMaxTimeMs = _storyMinTimeMs;
      }
      for (size_t index = 1; index < _storyDayPoints.size(); ++index)
      {
         const LocationPoint& point = _storyDayPoints[index];
         if (point.unixTimeMs < _storyMinTimeMs)
         {
            _storyMinTimeMs = point.unixTimeMs;
         }
         int64_t endTimeMs = point.endUnixTimeMs;
         if (endTimeMs < point.unixTimeMs)
         {
            endTimeMs = point.unixTimeMs;
         }
         if (endTimeMs > _storyMaxTimeMs)
         {
            _storyMaxTimeMs = endTimeMs;
         }
      }

      _pStorySlider->setEnabled(true);
      _pStoryPlayButton->setEnabled(true);

      const QDate storyDate = _pStoryDate->date();
      int64_t startDayEndMs = LastTimeOnCivilDate(
         _storyDayPoints,
         storyDate.year(),
         storyDate.month(),
         storyDate.day());
      if (startDayEndMs < _storyMinTimeMs)
      {
         startDayEndMs = _storyMinTimeMs;
      }

      const int32_t startDaySlider = SliderFromTimeCutoff(_storyMinTimeMs, _storyMaxTimeMs, startDayEndMs);
      _pStorySlider->blockSignals(true);
      _pStorySlider->setValue(startDaySlider);
      _pStorySlider->blockSignals(false);
      ApplyStoryCutoff();
   }

   void MainWindow::OnStoryDateChanged(const QDate& date)
   {
      (void)date;
      StopStoryPlayback();
      RefreshDisplayedPoints();
      UpdateStatusMessage();
   }

   void MainWindow::ApplyStoryCutoff(void)
   {
      if (_storyDayPoints.empty())
      {
         _pStoryTimeLabel->setText(QStringLiteral("—"));
         _pMapWidget->SetUntilTime(ShowAllUntilTimeMs);
         return;
      }

      const int64_t cutoffMs = TimeCutoffFromSlider(_storyMinTimeMs, _storyMaxTimeMs, _pStorySlider->value());
      _pMapWidget->SetUntilTime(cutoffMs);

      std::string timeText;
      const int32_t utcOffsetMinutes = _storyDayPoints[0].utcOffsetMinutes;
      FormatLocalTime(cutoffMs, utcOffsetMinutes, timeText);
      _pStoryTimeLabel->setText(QString::fromStdString(timeText));
   }

   void MainWindow::StopStoryPlayback(void)
   {
      _storyPlayback = StoryPlayback::Stopped;
      if (_pStoryTimer != nullptr)
      {
         _pStoryTimer->stop();
      }
      if (_pStoryPlayButton != nullptr)
      {
         _pStoryPlayButton->setText(tr("Play"));
      }
   }

   void MainWindow::OnStorySliderChanged(const int sliderValue)
   {
      (void)sliderValue;
      ApplyStoryCutoff();
   }

   void MainWindow::OnStoryPlayClicked(void)
   {
      if (_storyDayPoints.empty())
      {
         return;
      }

      if (_storyPlayback == StoryPlayback::Playing)
      {
         StopStoryPlayback();
         return;
      }

      if (_pStorySlider->value() >= StorySliderMax)
      {
         _pStorySlider->setValue(0);
      }

      _storyPlayback = StoryPlayback::Playing;
      _pStoryPlayButton->setText(tr("Pause"));
      _pStoryTimer->start();
   }

   void MainWindow::OnStoryTimerTick(void)
   {
      const int nextValue = _pStorySlider->value() + 1;
      if (nextValue >= StorySliderMax)
      {
         _pStorySlider->setValue(StorySliderMax);
         StopStoryPlayback();
         return;
      }

      _pStorySlider->setValue(nextValue);
   }

   QString MainWindow::FormatDuration(const int64_t durationMs) const
   {
      if (durationMs < 1000)
      {
         return tr("%1 ms").arg(durationMs);
      }
      if (durationMs < 60000)
      {
         return tr("%1 s").arg(durationMs / 1000);
      }

      const int64_t totalMinutes = durationMs / 60000;
      if (totalMinutes < 60)
      {
         return tr("%1 min").arg(totalMinutes);
      }

      const int64_t hours = totalMinutes / 60;
      const int64_t minutes = totalMinutes % 60;
      if (minutes == 0)
      {
         return tr("%1 h").arg(hours);
      }
      return tr("%1 h %2 min").arg(hours).arg(minutes);
   }

   void MainWindow::OnZoomInClicked(void)
   {
      _pMapWidget->ZoomIn();
   }

   void MainWindow::OnZoomOutClicked(void)
   {
      _pMapWidget->ZoomOut();
   }

   void MainWindow::OnZoomSliderChanged(const int zoomValue)
   {
      _pMapWidget->SetZoomLevel(zoomValue);
   }

   void MainWindow::OnMapZoomChanged(const int32_t zoom)
   {
      if (_pZoomSlider == nullptr)
      {
         return;
      }

      _pZoomSlider->blockSignals(true);
      _pZoomSlider->setValue(zoom);
      _pZoomSlider->blockSignals(false);
   }

   void MainWindow::OnLanguageActionTriggered(QAction* pAction)
   {
      if (pAction == nullptr)
      {
         return;
      }

      const int languageValue = pAction->data().toInt();
      const auto language = static_cast<AppLanguage>(languageValue);
      SaveLanguageSetting(language);
      ApplyAppLanguage(language);
   }

   void MainWindow::OnThemeActionTriggered(QAction* pAction)
   {
      if (pAction == nullptr)
      {
         return;
      }

      const int themeValue = pAction->data().toInt();
      const auto theme = static_cast<AppTheme>(themeValue);
      SaveThemeSetting(theme);
      ApplyAppTheme(theme);
   }

   void MainWindow::SyncZoomSlider(void)
   {
      if (_pMapWidget == nullptr)
      {
         return;
      }

      OnMapZoomChanged(_pMapWidget->Zoom());
   }

   void MainWindow::OnPointClicked(
      const double latitude,
      const double longitude,
      const int64_t unixTimeMs,
      const int32_t utcOffsetMinutes,
      const int64_t endUnixTimeMs,
      const PointSource source)
   {
      std::string timeText;
      FormatLocalTime(unixTimeMs, utcOffsetMinutes, timeText);
      _pTimeLabel->setText(QString::fromStdString(timeText));
      _pLatitudeLabel->setText(QString::number(latitude, 'f', 7));
      _pLongitudeLabel->setText(QString::number(longitude, 'f', 7));

      if ((source == PointSource::Visit) && (endUnixTimeMs > unixTimeMs))
      {
         std::string untilText;
         FormatLocalTime(endUnixTimeMs, utcOffsetMinutes, untilText);
         _pUntilLabel->setText(QString::fromStdString(untilText));
         _pDurationLabel->setText(FormatDuration(endUnixTimeMs - unixTimeMs));
         return;
      }

      _pUntilLabel->setText(QStringLiteral("—"));
      _pDurationLabel->setText(QStringLiteral("—"));
   }

   void MainWindow::OnPointCleared(void)
   {
      _pTimeLabel->setText(QStringLiteral("—"));
      _pUntilLabel->setText(QStringLiteral("—"));
      _pDurationLabel->setText(QStringLiteral("—"));
      _pLatitudeLabel->setText(QStringLiteral("—"));
      _pLongitudeLabel->setText(QStringLiteral("—"));
   }
} // namespace LocationHistory
