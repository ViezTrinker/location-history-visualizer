/*!
 *\file main_window.cpp
 *\brief Main application window with filters, map, and About menu
 */

#include "main_window.h"

#include <cstdint>
#include <string>
#include <string_view>

#include <QApplication>
#include <QDate>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTime>
#include <QVBoxLayout>
#include <QWidget>

#include "about_dialog.h"
#include "app_language.h"
#include "civil_time.h"
#include "heatmap_renderer.h"
#include "json_loader.h"
#include "load_result.h"
#include "location_filter.h"
#include "map_widget.h"
#include "tile_math.h"
#include "version.h"

namespace LocationHistory
{
   namespace
   {
      inline constexpr int32_t ZoomButtonSizePx = 32;
      inline constexpr int32_t ZoomPanelWidthPx = 48;

      QString LastJsonSettingsKey(void)
      {
         return QStringLiteral("lastJsonPath");
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
   } // namespace

   MainWindow::MainWindow(QWidget* pParent)
      : QMainWindow(pParent)
      , _pMapWidget(nullptr)
      , _pFileMenu(nullptr)
      , _pOpenAction(nullptr)
      , _pQuitAction(nullptr)
      , _pHelpMenu(nullptr)
      , _pAboutAction(nullptr)
      , _pOpenButton(nullptr)
      , _pFileLabel(nullptr)
      , _pDateGroup(nullptr)
      , _pFromDateLabel(nullptr)
      , _pToDateLabel(nullptr)
      , _pFromDate(nullptr)
      , _pToDate(nullptr)
      , _pWeekdayGroup(nullptr)
      , _pWeekdayBoxes{}
      , _pTimeGroup(nullptr)
      , _pFromTimeLabel(nullptr)
      , _pToTimeLabel(nullptr)
      , _pFromTime(nullptr)
      , _pToTime(nullptr)
      , _pModeGroup(nullptr)
      , _pDisplayMode(nullptr)
      , _pHeatScaleCaption(nullptr)
      , _pHeatScaleSlider(nullptr)
      , _pHeatScaleValueLabel(nullptr)
      , _pInfoGroup(nullptr)
      , _pWhenCaption(nullptr)
      , _pLatitudeCaption(nullptr)
      , _pLongitudeCaption(nullptr)
      , _pLanguageGroup(nullptr)
      , _pLanguageCombo(nullptr)
      , _pZoomInButton(nullptr)
      , _pZoomOutButton(nullptr)
      , _pZoomSlider(nullptr)
      , _pTimeLabel(nullptr)
      , _pLatitudeLabel(nullptr)
      , _pLongitudeLabel(nullptr)
   {
      setWindowTitle(QString::fromUtf8(AppName.data(), static_cast<int>(AppName.size())));
      resize(1280, 800);
      BuildMenus();
      BuildUi();
      RetranslateUi();
      OnPointCleared();
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

      _pOpenButton = new QPushButton(pSidePanel);
      connect(_pOpenButton, &QPushButton::clicked, this, &MainWindow::OnOpenClicked);
      _pFileLabel = new QLabel(pSidePanel);
      _pFileLabel->setWordWrap(true);
      pSideLayout->addWidget(_pOpenButton);
      pSideLayout->addWidget(_pFileLabel);

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
      for (size_t weekdayIndex = 0; weekdayIndex < WeekdayCount; ++weekdayIndex)
      {
         QCheckBox* pBox = new QCheckBox(_pWeekdayGroup);
         pBox->setChecked(true);
         connect(pBox, &QCheckBox::stateChanged, this, &MainWindow::OnFiltersChanged);
         _pWeekdayBoxes[weekdayIndex] = pBox;
         pWeekdayLayout->addWidget(pBox);
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
      _pDisplayMode->addItem(QString(), static_cast<int>(DisplayMode::AllPoints));
      _pDisplayMode->addItem(QString(), static_cast<int>(DisplayMode::Clustered));
      _pDisplayMode->addItem(QString(), static_cast<int>(DisplayMode::Heatmap));
      _pDisplayMode->addItem(QString(), static_cast<int>(DisplayMode::Blur));
      connect(_pDisplayMode, &QComboBox::currentIndexChanged, this, &MainWindow::OnDisplayModeChanged);
      pModeLayout->addWidget(_pDisplayMode);

      _pHeatScaleCaption = new QLabel(_pModeGroup);
      _pHeatScaleSlider = new QSlider(Qt::Horizontal, _pModeGroup);
      _pHeatScaleSlider->setRange(HeatScaleSliderMin, HeatScaleSliderMax);
      _pHeatScaleSlider->setValue(HeatScaleSliderMin);
      connect(_pHeatScaleSlider, &QSlider::valueChanged, this, &MainWindow::OnHeatScaleChanged);
      _pHeatScaleValueLabel = new QLabel(_pModeGroup);
      pModeLayout->addWidget(_pHeatScaleCaption);
      pModeLayout->addWidget(_pHeatScaleSlider);
      pModeLayout->addWidget(_pHeatScaleValueLabel);
      pSideLayout->addWidget(_pModeGroup);

      _pInfoGroup = new QGroupBox(pSidePanel);
      QFormLayout* pInfoLayout = new QFormLayout(_pInfoGroup);
      _pWhenCaption = new QLabel(_pInfoGroup);
      _pLatitudeCaption = new QLabel(_pInfoGroup);
      _pLongitudeCaption = new QLabel(_pInfoGroup);
      _pTimeLabel = new QLabel(QStringLiteral("—"), _pInfoGroup);
      _pLatitudeLabel = new QLabel(QStringLiteral("—"), _pInfoGroup);
      _pLongitudeLabel = new QLabel(QStringLiteral("—"), _pInfoGroup);
      _pTimeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      _pLatitudeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      _pLongitudeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      pInfoLayout->addRow(_pWhenCaption, _pTimeLabel);
      pInfoLayout->addRow(_pLatitudeCaption, _pLatitudeLabel);
      pInfoLayout->addRow(_pLongitudeCaption, _pLongitudeLabel);
      pSideLayout->addWidget(_pInfoGroup);

      _pLanguageGroup = new QGroupBox(pSidePanel);
      QVBoxLayout* pLanguageLayout = new QVBoxLayout(_pLanguageGroup);
      _pLanguageCombo = new QComboBox(_pLanguageGroup);
      FillLanguageCombo();
      connect(_pLanguageCombo, &QComboBox::currentIndexChanged, this, &MainWindow::OnLanguageChanged);
      pLanguageLayout->addWidget(_pLanguageCombo);
      pSideLayout->addWidget(_pLanguageGroup);
      pSideLayout->addStretch();

      _pMapWidget = new MapWidget(pCentral);
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

      QHBoxLayout* pRootLayout = new QHBoxLayout(pCentral);
      pRootLayout->addWidget(pSidePanel);
      pRootLayout->addWidget(_pMapWidget, 1);
      pRootLayout->addWidget(pZoomPanel);

      UpdateHeatScaleControls();
   }

   void MainWindow::FillLanguageCombo(void)
   {
      const AppLanguage currentLanguage = LoadLanguageSetting();
      _pLanguageCombo->blockSignals(true);
      _pLanguageCombo->clear();
      for (uint8_t languageValue = 0; languageValue < AppLanguageCount; ++languageValue)
      {
         const auto language = static_cast<AppLanguage>(languageValue);
         const std::string_view nativeName = LanguageNativeName(language);
         _pLanguageCombo->addItem(
            QString::fromUtf8(nativeName.data(), static_cast<int>(nativeName.size())),
            static_cast<int>(languageValue));
         if (language == currentLanguage)
         {
            _pLanguageCombo->setCurrentIndex(static_cast<int>(languageValue));
         }
      }
      _pLanguageCombo->blockSignals(false);
   }

   void MainWindow::RetranslateUi(void)
   {
      _pFileMenu->setTitle(tr("&File"));
      _pOpenAction->setText(tr("Open..."));
      _pQuitAction->setText(tr("E&xit"));
      _pHelpMenu->setTitle(tr("&Help"));
      _pAboutAction->setText(tr("About"));
      _pOpenButton->setText(tr("Open JSON..."));
      _pDateGroup->setTitle(tr("Date"));
      _pFromDateLabel->setText(tr("From"));
      _pToDateLabel->setText(tr("To"));
      _pWeekdayGroup->setTitle(tr("Weekday"));
      for (size_t weekdayIndex = 0; weekdayIndex < WeekdayCount; ++weekdayIndex)
      {
         _pWeekdayBoxes[weekdayIndex]->setText(WeekdayText(weekdayIndex));
      }
      _pTimeGroup->setTitle(tr("Time of day"));
      _pFromTimeLabel->setText(tr("From"));
      _pToTimeLabel->setText(tr("To"));
      _pModeGroup->setTitle(tr("Display"));
      _pDisplayMode->setItemText(0, tr("All points"));
      _pDisplayMode->setItemText(1, tr("Cluster"));
      _pDisplayMode->setItemText(2, tr("Heatmap"));
      _pDisplayMode->setItemText(3, tr("Blur"));
      _pHeatScaleCaption->setText(tr("Scaling"));
      const QString heatScaleTip =
         tr("Raises rarer places in Heatmap and Blur. High values saturate frequent places.");
      _pHeatScaleCaption->setToolTip(heatScaleTip);
      _pHeatScaleSlider->setToolTip(heatScaleTip);
      _pInfoGroup->setTitle(tr("Point info"));
      _pWhenCaption->setText(tr("When"));
      _pLatitudeCaption->setText(tr("Latitude"));
      _pLongitudeCaption->setText(tr("Longitude"));
      _pLanguageGroup->setTitle(tr("Language"));
      _pZoomInButton->setToolTip(tr("Zoom in"));
      _pZoomOutButton->setToolTip(tr("Zoom out"));
      _pZoomSlider->setToolTip(tr("Zoom"));
      UpdateFileLabel();
      UpdateHeatScaleControls();
      UpdateStatusMessage();
   }

   void MainWindow::UpdateFileLabel(void)
   {
      if (_loadedFilePath.isEmpty())
      {
         _pFileLabel->setText(tr("No file loaded"));
         return;
      }

      _pFileLabel->setText(_loadedFilePath);
   }

   void MainWindow::UpdateStatusMessage(void)
   {
      if (_loadedFilePath.isEmpty())
      {
         statusBar()->showMessage(tr("Ready"));
         return;
      }

      statusBar()->showMessage(tr("%1 points shown").arg(_filteredPoints.size()));
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
      _pMapWidget->SetPoints(_filteredPoints);
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
      const QString path = QFileDialog::getOpenFileName(
         this,
         tr("Open Timeline JSON"),
         LastJsonDialogPath(),
         tr("JSON files (*.json);;All files (*.*)"));
      if (path.isEmpty())
      {
         return;
      }

      QApplication::setOverrideCursor(Qt::WaitCursor);
      const LoadResult result = LoadFromFile(path.toStdString(), _allPoints);
      QApplication::restoreOverrideCursor();

      if (IsErr(result))
      {
         QMessageBox::warning(this, tr("Load failed"), LoadResultMessage(result));
         return;
      }

      QSettings settings;
      settings.setValue(LastJsonSettingsKey(), path);
      _loadedFilePath = path;
      UpdateFileLabel();
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

   void MainWindow::OnFiltersChanged(void)
   {
      ApplyCurrentFilter();
   }

   void MainWindow::OnDisplayModeChanged(const int index)
   {
      const int modeValue = _pDisplayMode->itemData(index).toInt();
      _pMapWidget->SetDisplayMode(static_cast<DisplayMode>(modeValue));
      UpdateHeatScaleControls();
   }

   void MainWindow::OnHeatScaleChanged(const int sliderValue)
   {
      const float heatScale = HeatScaleFromSlider(sliderValue);
      _pMapWidget->SetHeatScale(heatScale);
      _pHeatScaleValueLabel->setText(tr("×%1").arg(heatScale, 0, 'f', 1));
   }

   void MainWindow::UpdateHeatScaleControls(void)
   {
      const int modeValue = _pDisplayMode->currentData().toInt();
      const auto displayMode = static_cast<DisplayMode>(modeValue);
      const bool intensityMode = (displayMode == DisplayMode::Heatmap) || (displayMode == DisplayMode::Blur);
      _pHeatScaleSlider->setEnabled(intensityMode);
      _pHeatScaleValueLabel->setEnabled(intensityMode);

      const float heatScale = HeatScaleFromSlider(_pHeatScaleSlider->value());
      _pHeatScaleValueLabel->setText(tr("×%1").arg(heatScale, 0, 'f', 1));
      _pMapWidget->SetHeatScale(heatScale);
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

   void MainWindow::OnLanguageChanged(const int index)
   {
      const int languageValue = _pLanguageCombo->itemData(index).toInt();
      const auto language = static_cast<AppLanguage>(languageValue);
      SaveLanguageSetting(language);
      ApplyAppLanguage(language);
   }

   void MainWindow::SyncZoomSlider(void)
   {
      if (_pMapWidget == nullptr)
      {
         return;
      }

      OnMapZoomChanged(_pMapWidget->Zoom());
   }

   void MainWindow::OnPointClicked(const double latitude, const double longitude, const int64_t unixTimeMs, const int32_t utcOffsetMinutes)
   {
      std::string timeText;
      FormatLocalTime(unixTimeMs, utcOffsetMinutes, timeText);
      _pTimeLabel->setText(QString::fromStdString(timeText));
      _pLatitudeLabel->setText(QString::number(latitude, 'f', 7));
      _pLongitudeLabel->setText(QString::number(longitude, 'f', 7));
   }

   void MainWindow::OnPointCleared(void)
   {
      _pTimeLabel->setText(QStringLiteral("—"));
      _pLatitudeLabel->setText(QStringLiteral("—"));
      _pLongitudeLabel->setText(QStringLiteral("—"));
   }
} // namespace LocationHistory
