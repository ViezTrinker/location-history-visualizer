/*!
 *\file about_dialog.h
 *\brief About dialog with version, date, and project links
 */

#ifndef ABOUT_DIALOG_H
#define ABOUT_DIALOG_H

#include <QDialog>

namespace LocationHistory
{
   class AboutDialog : public QDialog
   {
         Q_OBJECT

      public:
         explicit AboutDialog(QWidget* pParent = nullptr);
   };
} // namespace LocationHistory

#endif // ABOUT_DIALOG_H
