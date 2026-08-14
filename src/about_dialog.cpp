/*!
 *\file about_dialog.cpp
 *\brief About dialog with version, date, and project links
 */

#include "about_dialog.h"

#include <string_view>

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

#include "version.h"

namespace LocationHistory
{
   namespace
   {
      QString MakeLink(const std::string_view url)
      {
         const QString urlText = QString::fromUtf8(url.data(), static_cast<int>(url.size()));
         return QStringLiteral("<a href=\"%1\">%1</a>").arg(urlText);
      }

      QLabel* MakeInfoLabel(QWidget* pParent, const QString& text)
      {
         QLabel* pLabel = new QLabel(text, pParent);
         pLabel->setTextFormat(Qt::RichText);
         pLabel->setOpenExternalLinks(true);
         pLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
         return pLabel;
      }
   } // namespace

   AboutDialog::AboutDialog(QWidget* pParent)
      : QDialog(pParent)
   {
      setWindowTitle(tr("About"));
      setModal(true);

      const QString title = QString::fromUtf8(AppName.data(), static_cast<int>(AppName.size()));
      const QString version = QString::fromUtf8(AppVersion.data(), static_cast<int>(AppVersion.size()));
      const QString date = QString::fromUtf8(ReleaseDate.data(), static_cast<int>(ReleaseDate.size()));
      const QString authorName = QString::fromUtf8(AuthorName.data(), static_cast<int>(AuthorName.size()));

      QLabel* pTitleLabel = new QLabel(QStringLiteral("<h2>%1</h2>").arg(title), this);
      QLabel* pVersionLabel = MakeInfoLabel(this, tr("<b>Version:</b> %1").arg(version));
      QLabel* pDateLabel = MakeInfoLabel(this, tr("<b>Datum:</b> %1").arg(date));
      QLabel* pAuthorLabel = MakeInfoLabel(this, tr("<b>%1:</b> %2").arg(authorName, MakeLink(AuthorUrl)));
      QLabel* pRepoLabel = MakeInfoLabel(this, tr("<b>Github Repository:</b> %1").arg(MakeLink(RepositoryUrl)));

      QDialogButtonBox* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
      connect(pButtons, &QDialogButtonBox::accepted, this, &AboutDialog::accept);

      QVBoxLayout* pLayout = new QVBoxLayout(this);
      pLayout->addWidget(pTitleLabel);
      pLayout->addWidget(pVersionLabel);
      pLayout->addWidget(pDateLabel);
      pLayout->addWidget(pAuthorLabel);
      pLayout->addWidget(pRepoLabel);
      pLayout->addWidget(pButtons);
   }
} // namespace LocationHistory
