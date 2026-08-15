/*!
 *\file map_display_dialog.cpp
 *\brief Dialog for drawn-point limit and point radius
 */

#include "map_display_dialog.h"

#include <cstdint>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

#include "map_display_settings.h"

namespace LocationHistory
{
   MapDisplayDialog::MapDisplayDialog(
      const int32_t pointRadiusPx,
      const int32_t drawnPointLimit,
      QWidget* pParent)
      : QDialog(pParent)
      , _pPointRadiusSpin(nullptr)
      , _pDrawnPointLimitSpin(nullptr)
   {
      setWindowTitle(tr("Map display"));
      setModal(true);

      _pPointRadiusSpin = new QSpinBox(this);
      _pPointRadiusSpin->setRange(MinPointRadiusPx, MaxPointRadiusPx);
      _pPointRadiusSpin->setSingleStep(1);
      _pPointRadiusSpin->setValue(ClampPointRadiusPx(pointRadiusPx));
      _pPointRadiusSpin->setSuffix(tr(" px"));

      _pDrawnPointLimitSpin = new QSpinBox(this);
      _pDrawnPointLimitSpin->setRange(MinDrawnPointLimit, MaxDrawnPointLimit);
      _pDrawnPointLimitSpin->setSingleStep(DrawnPointLimitSpinStep);
      _pDrawnPointLimitSpin->setAccelerated(true);
      _pDrawnPointLimitSpin->setGroupSeparatorShown(true);
      _pDrawnPointLimitSpin->setValue(ClampDrawnPointLimit(drawnPointLimit));

      QFormLayout* pFormLayout = new QFormLayout();
      pFormLayout->addRow(tr("Point size"), _pPointRadiusSpin);
      pFormLayout->addRow(tr("Maximum drawn points"), _pDrawnPointLimitSpin);

      QDialogButtonBox* pButtons = new QDialogButtonBox(
         QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
         this);
      connect(pButtons, &QDialogButtonBox::accepted, this, &MapDisplayDialog::accept);
      connect(pButtons, &QDialogButtonBox::rejected, this, &MapDisplayDialog::reject);

      QVBoxLayout* pLayout = new QVBoxLayout(this);
      pLayout->addLayout(pFormLayout);
      pLayout->addWidget(pButtons);
   }

   int32_t MapDisplayDialog::PointRadiusPx(void) const
   {
      if (_pPointRadiusSpin == nullptr)
      {
         return DefaultPointRadiusPx;
      }
      return ClampPointRadiusPx(static_cast<int32_t>(_pPointRadiusSpin->value()));
   }

   int32_t MapDisplayDialog::DrawnPointLimit(void) const
   {
      if (_pDrawnPointLimitSpin == nullptr)
      {
         return DefaultDrawnPointLimit;
      }
      return ClampDrawnPointLimit(static_cast<int32_t>(_pDrawnPointLimitSpin->value()));
   }
} // namespace LocationHistory
