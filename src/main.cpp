/*!
 *\file main.cpp
 *\brief Application entry point
 */

#include <QApplication>
#include <QIcon>

#include "app_language.h"
#include "app_theme.h"
#include "main_window.h"
#include "version.h"

int main(int argc, char* argv[])
{
   QApplication application(argc, argv);
   application.setApplicationName(QString::fromUtf8(
      LocationHistory::AppName.data(),
      static_cast<int>(LocationHistory::AppName.size())));
   application.setOrganizationName(QString::fromUtf8(
      LocationHistory::AuthorName.data(),
      static_cast<int>(LocationHistory::AuthorName.size())));
   application.setWindowIcon(QIcon(QStringLiteral(":/app.ico")));

   LocationHistory::ApplySavedAppLanguage();
   LocationHistory::ApplySavedAppTheme();

   LocationHistory::MainWindow window;
   window.show();
   return application.exec();
}
