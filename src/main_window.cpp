/*!
 *\file main_window.cpp
 *\brief Main application window with filters, map, and About menu
 */

#include "main_window.h"

#include <cstdint>
#include <string>
#include <string_view>

#include <QAction>
#include <QApplication>
#include <QDate>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QStatusBar>
#include <QTime>
#include <QVBoxLayout>
#include <QWidget>

#include "about_dialog.h"
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

      std::string_view WeekdayLabel(const size_t weekdayIndex)
      {
         switch (weekdayIndex)
         {
            case 0:
               return "Mo";
            case 1:
               return "Di";
            case 2:
               return "Mi";
            case 3:
               return "Do";
            case 4:
               return "Fr";
            case 5:
               return "Sa";
            default:
               return "So";
         }
      }

      QString LoadResultMessage(const LoadResult result)
      {
         if (result == LoadResult::FileNotFound)
         {
            return QObject::tr("Datei wurde nicht gefunden.");
         }
         if (result == LoadResult::InvalidJson)
         {
            return QObject::tr("Die JSON-Datei ist ungültig.");
         }
         if (result == LoadResult::NoPoints)
         {
            return QObject::tr("Die Datei enthält keine Standortdaten.");
         }
         return QObject::tr("Die Datei konnte nicht geladen werden.");
      }

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
      , _pOpenButton(nullptr)
      , _pFileLabel(nullptr)
      , _pFromDate(nullptr)
      , _pToDate(nullptr)
      , _pWeekdayBoxes{}
      , _pFromTime(nullptr)
      , _pToTime(nullptr)
      , _pDisplayMode(nullptr)
      , _pHeatScaleSlider(nullptr)
      , _pHeatScaleValueLabel(nullptr)
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
      OnPointCleared();
   }

   void MainWindow::BuildMenus(void)
   {
      QMenu* pFileMenu = menuBar()->addMenu(tr("&Datei"));
      QAction* pOpenAction = pFileMenu->addAction(tr("Öffnen..."));
      pOpenAction->setShortcut(QKeySequence::Open);
      connect(pOpenAction, &QAction::triggered, this, &MainWindow::OnOpenClicked);

      QAction* pQuitAction = pFileMenu->addAction(tr("Beenden"));
      pQuitAction->setShortcut(QKeySequence::Quit);
      connect(pQuitAction, &QAction::triggered, this, &MainWindow::close);

      QMenu* pHelpMenu = menuBar()->addMenu(tr("&Hilfe"));
      QAction* pAboutAction = pHelpMenu->addAction(tr("About"));
      connect(pAboutAction, &QAction::triggered, this, &MainWindow::OnAbout);
   }

   void MainWindow::BuildUi(void)
   {
      QWidget* pCentral = new QWidget(this);
      setCentralWidget(pCentral);

      QWidget* pSidePanel = new QWidget(pCentral);
      pSidePanel->setFixedWidth(280);
      QVBoxLayout* pSideLayout = new QVBoxLayout(pSidePanel);

      _pOpenButton = new QPushButton(tr("JSON öffnen..."), pSidePanel);
      connect(_pOpenButton, &QPushButton::clicked, this, &MainWindow::OnOpenClicked);
      _pFileLabel = new QLabel(tr("Keine Datei geladen"), pSidePanel);
      _pFileLabel->setWordWrap(true);
      pSideLayout->addWidget(_pOpenButton);
      pSideLayout->addWidget(_pFileLabel);

      QGroupBox* pDateGroup = new QGroupBox(tr("Datum"), pSidePanel);
      QFormLayout* pDateLayout = new QFormLayout(pDateGroup);
      _pFromDate = new QDateEdit(QDate(1970, 1, 1), pDateGroup);
      _pToDate = new QDateEdit(QDate(9999, 12, 31), pDateGroup);
      _pFromDate->setCalendarPopup(true);
      _pToDate->setCalendarPopup(true);
      _pFromDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
      _pToDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
      connect(_pFromDate, &QDateEdit::dateChanged, this, &MainWindow::OnFiltersChanged);
      connect(_pToDate, &QDateEdit::dateChanged, this, &MainWindow::OnFiltersChanged);
      pDateLayout->addRow(tr("Von"), _pFromDate);
      pDateLayout->addRow(tr("Bis"), _pToDate);
      pSideLayout->addWidget(pDateGroup);

      QGroupBox* pWeekdayGroup = new QGroupBox(tr("Wochentag"), pSidePanel);
      QHBoxLayout* pWeekdayLayout = new QHBoxLayout(pWeekdayGroup);
      for (size_t weekdayIndex = 0; weekdayIndex < WeekdayCount; ++weekdayIndex)
      {
         const std::string_view weekdayLabel = WeekdayLabel(weekdayIndex);
         QCheckBox* pBox = new QCheckBox(
            QString::fromUtf8(weekdayLabel.data(), static_cast<int>(weekdayLabel.size())),
            pWeekdayGroup);
         pBox->setChecked(true);
         connect(pBox, &QCheckBox::stateChanged, this, &MainWindow::OnFiltersChanged);
         _pWeekdayBoxes[weekdayIndex] = pBox;
         pWeekdayLayout->addWidget(pBox);
      }
      pSideLayout->addWidget(pWeekdayGroup);

      QGroupBox* pTimeGroup = new QGroupBox(tr("Uhrzeit"), pSidePanel);
      QFormLayout* pTimeLayout = new QFormLayout(pTimeGroup);
      _pFromTime = new QTimeEdit(QTime(0, 0), pTimeGroup);
      _pToTime = new QTimeEdit(QTime(23, 59), pTimeGroup);
      _pFromTime->setDisplayFormat(QStringLiteral("HH:mm"));
      _pToTime->setDisplayFormat(QStringLiteral("HH:mm"));
      connect(_pFromTime, &QTimeEdit::timeChanged, this, &MainWindow::OnFiltersChanged);
      connect(_pToTime, &QTimeEdit::timeChanged, this, &MainWindow::OnFiltersChanged);
      pTimeLayout->addRow(tr("Von"), _pFromTime);
      pTimeLayout->addRow(tr("Bis"), _pToTime);
      pSideLayout->addWidget(pTimeGroup);

      QGroupBox* pModeGroup = new QGroupBox(tr("Darstellung"), pSidePanel);
      QVBoxLayout* pModeLayout = new QVBoxLayout(pModeGroup);
      _pDisplayMode = new QComboBox(pModeGroup);
      _pDisplayMode->addItem(tr("Alle Punkte"), static_cast<int>(DisplayMode::AllPoints));
      _pDisplayMode->addItem(tr("Cluster"), static_cast<int>(DisplayMode::Clustered));
      _pDisplayMode->addItem(tr("Heatmap"), static_cast<int>(DisplayMode::Heatmap));
      _pDisplayMode->addItem(tr("Blur"), static_cast<int>(DisplayMode::Blur));
      connect(_pDisplayMode, &QComboBox::currentIndexChanged, this, &MainWindow::OnDisplayModeChanged);
      pModeLayout->addWidget(_pDisplayMode);

      QLabel* pHeatScaleCaption = new QLabel(tr("Skalierung"), pModeGroup);
      pHeatScaleCaption->setToolTip(tr("Hebt seltenere Orte in Heatmap und Blur an. Hohe Werte sättigen häufige Orte."));
      _pHeatScaleSlider = new QSlider(Qt::Horizontal, pModeGroup);
      _pHeatScaleSlider->setRange(HeatScaleSliderMin, HeatScaleSliderMax);
      _pHeatScaleSlider->setValue(HeatScaleSliderMin);
      _pHeatScaleSlider->setToolTip(pHeatScaleCaption->toolTip());
      connect(_pHeatScaleSlider, &QSlider::valueChanged, this, &MainWindow::OnHeatScaleChanged);
      _pHeatScaleValueLabel = new QLabel(pModeGroup);
      pModeLayout->addWidget(pHeatScaleCaption);
      pModeLayout->addWidget(_pHeatScaleSlider);
      pModeLayout->addWidget(_pHeatScaleValueLabel);
      pSideLayout->addWidget(pModeGroup);

      QGroupBox* pInfoGroup = new QGroupBox(tr("Punktinfo"), pSidePanel);
      QFormLayout* pInfoLayout = new QFormLayout(pInfoGroup);
      _pTimeLabel = new QLabel(QStringLiteral("—"), pInfoGroup);
      _pLatitudeLabel = new QLabel(QStringLiteral("—"), pInfoGroup);
      _pLongitudeLabel = new QLabel(QStringLiteral("—"), pInfoGroup);
      _pTimeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      _pLatitudeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      _pLongitudeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
      pInfoLayout->addRow(tr("Wann"), _pTimeLabel);
      pInfoLayout->addRow(tr("Latitude"), _pLatitudeLabel);
      pInfoLayout->addRow(tr("Longitude"), _pLongitudeLabel);
      pSideLayout->addWidget(pInfoGroup);
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
      _pZoomInButton->setToolTip(tr("Hineinzoomen"));
      _pZoomOutButton->setToolTip(tr("Herauszoomen"));
      _pZoomSlider->setToolTip(tr("Zoom"));
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
      statusBar()->showMessage(tr("Bereit"));
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
      statusBar()->showMessage(tr("%1 Punkte angezeigt").arg(_filteredPoints.size()));
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
         tr("Timeline JSON öffnen"),
         LastJsonDialogPath(),
         tr("JSON-Dateien (*.json);;Alle Dateien (*.*)"));
      if (path.isEmpty())
      {
         return;
      }

      QApplication::setOverrideCursor(Qt::WaitCursor);
      const LoadResult result = LoadFromFile(path.toStdString(), _allPoints);
      QApplication::restoreOverrideCursor();

      if (IsErr(result))
      {
         QMessageBox::warning(this, tr("Laden fehlgeschlagen"), LoadResultMessage(result));
         return;
      }

      QSettings settings;
      settings.setValue(LastJsonSettingsKey(), path);
      _pFileLabel->setText(path);
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
