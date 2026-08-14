/*!
 *\file app_language.cpp
 *\brief UI language selection and Qt translator loading
 */

#include "app_language.h"

#include <string_view>

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QString>
#include <QTranslator>

namespace LocationHistory
{
   namespace
   {
      QTranslator appTranslator;
      QTranslator qtTranslator;

      QString LanguageSettingsKey(void)
      {
         return QStringLiteral("language");
      }

      QLocale LocaleForLanguage(const AppLanguage language)
      {
         if (language == AppLanguage::German)
         {
            return QLocale(QLocale::German, QLocale::Germany);
         }
         if (language == AppLanguage::Spanish)
         {
            return QLocale(QLocale::Spanish, QLocale::Spain);
         }
         if (language == AppLanguage::French)
         {
            return QLocale(QLocale::French, QLocale::France);
         }
         return QLocale(QLocale::English, QLocale::UnitedStates);
      }

      QString LanguageCodeString(const AppLanguage language)
      {
         const std::string_view code = LanguageCode(language);
         return QString::fromUtf8(code.data(), static_cast<int>(code.size()));
      }

      void LoadQtBaseTranslator(const AppLanguage language)
      {
         if (language == AppLanguage::English)
         {
            return;
         }

         const QString qtQmName = QStringLiteral("qtbase_%1").arg(LanguageCodeString(language));
         const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
         if (qtTranslator.load(qtQmName, qtTranslationsPath))
         {
            QCoreApplication::installTranslator(&qtTranslator);
            return;
         }

         const QString localTranslationsPath =
            QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
         if (qtTranslator.load(qtQmName, localTranslationsPath))
         {
            QCoreApplication::installTranslator(&qtTranslator);
         }
      }

      void LoadAppTranslator(const AppLanguage language)
      {
         if (language == AppLanguage::English)
         {
            return;
         }

         const QString appQmName =
            QStringLiteral("location_history_visualizer_%1").arg(LanguageCodeString(language));
         if (appTranslator.load(appQmName, QStringLiteral(":/i18n")))
         {
            QCoreApplication::installTranslator(&appTranslator);
         }
      }
   } // namespace

   AppLanguage LoadLanguageSetting(void)
   {
      QSettings settings;
      const QString storedCode = settings.value(LanguageSettingsKey()).toString();
      if (storedCode.isEmpty())
      {
         return DefaultAppLanguage;
      }

      return LanguageFromCode(storedCode.toStdString());
   }

   void SaveLanguageSetting(const AppLanguage language)
   {
      QSettings settings;
      settings.setValue(LanguageSettingsKey(), LanguageCodeString(language));
   }

   void ApplyAppLanguage(const AppLanguage language)
   {
      QCoreApplication::removeTranslator(&appTranslator);
      QCoreApplication::removeTranslator(&qtTranslator);
      QLocale::setDefault(LocaleForLanguage(language));
      LoadAppTranslator(language);
      LoadQtBaseTranslator(language);
   }

   void ApplySavedAppLanguage(void)
   {
      ApplyAppLanguage(LoadLanguageSetting());
   }
} // namespace LocationHistory
