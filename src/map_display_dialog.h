/*!
 *\file map_display_dialog.h
 *\brief Dialog for drawn-point limit and point radius
 */

#ifndef MAP_DISPLAY_DIALOG_H
#define MAP_DISPLAY_DIALOG_H

#include <cstdint>

#include <QDialog>
#include <QSpinBox>
#include <QWidget>

namespace LocationHistory
{
   class MapDisplayDialog : public QDialog
   {
         Q_OBJECT

      public:
         explicit MapDisplayDialog(
            int32_t pointRadiusPx,
            int32_t drawnPointLimit,
            QWidget* pParent = nullptr);

         /*!
          *\brief Returns the chosen point radius in pixels
          */
         int32_t PointRadiusPx(void) const;

         /*!
          *\brief Returns the chosen maximum number of drawn points
          */
         int32_t DrawnPointLimit(void) const;

      private:
         QSpinBox* _pPointRadiusSpin;
         QSpinBox* _pDrawnPointLimitSpin;
   };
} // namespace LocationHistory

#endif // MAP_DISPLAY_DIALOG_H
