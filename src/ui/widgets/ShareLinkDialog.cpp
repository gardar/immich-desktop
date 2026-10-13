#include "ui/widgets/ShareLinkDialog.h"
#include "core/ImmichClient.h"
#include "ui/IconUtils.h"

#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace Aurora {
ShareLinkDialog::ShareLinkDialog(ImmichClient *client, const ImmichAsset &asset, QWidget *parent)
    : QDialog(parent), m_client(client), m_asset(asset),
      m_link(new QLineEdit(this)), m_status(new QLabel(this)),
      m_create(new QPushButton(tr("Create link"), this)),
      m_copy(new QPushButton(tr("Copy link"), this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground);
    setWindowTitle(tr("Share media"));
    resize(600, 340);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(14);
    auto *title = new QLabel(tr("Share with a link"), this);
    title->setProperty("section", true);
    layout->addWidget(title);
    auto *name = new QLabel(asset.fileName.isEmpty() ? tr("Photo or video") : asset.fileName, this);
    name->setTextFormat(Qt::PlainText);
    name->setWordWrap(true);
    layout->addWidget(name);
    auto *help = new QLabel(tr("The link uses your Immich URL from server settings, even when the app is connected locally."), this);
    help->setWordWrap(true);
    help->setProperty("subheading", true);
    layout->addWidget(help);
    auto *passwordLabel = new QLabel(tr("Password (optional)"), this);
    m_password = new QLineEdit(this);
    m_password->setObjectName(QStringLiteral("sharePasswordField"));
    m_password->setAccessibleName(tr("Share link password"));
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(tr("Leave empty to share without a password"));
    passwordLabel->setBuddy(m_password);
    layout->addWidget(passwordLabel);
    auto *passwordRow = new QHBoxLayout;
    m_showPassword = new QCheckBox(tr("Show password"), this);
    m_showPassword->setObjectName(QStringLiteral("showSharePassword"));
    passwordRow->addWidget(m_password, 1);
    passwordRow->addWidget(m_showPassword);
    layout->addLayout(passwordRow);
    connect(m_showPassword, &QCheckBox::toggled, this, [this](bool show) {
        m_password->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    });
    auto *row = new QHBoxLayout;
    m_link->setObjectName(QStringLiteral("sharedLinkField"));
    m_link->setReadOnly(true);
    m_link->setAccessibleName(tr("Share link"));
    m_link->setPlaceholderText(tr("Create a link to share this media…"));
    m_copy->setObjectName(QStringLiteral("copySharedLinkButton"));
    configureActionButton(m_copy, QStringLiteral(":/icons/copy.svg"));
    m_copy->setEnabled(false);
    row->addWidget(m_link, 1);
    row->addWidget(m_copy);
    layout->addLayout(row);
    m_status->setTextFormat(Qt::PlainText);
    m_status->setWordWrap(true);
    m_status->setProperty("subheading", true);
    layout->addWidget(m_status);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->addButton(m_create, QDialogButtonBox::ActionRole);
    m_create->setObjectName(QStringLiteral("createSharedLinkButton"));
    configureActionButton(m_create, QStringLiteral(":/icons/external-link.svg"));
    m_create->setEnabled(!asset.isLocalPending() && client->isConfigured());
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_create, &QPushButton::clicked, this, [this] {
        m_requestId = QUuid::createUuid();
        m_create->setEnabled(false);
        m_passwordProtected = !m_password->text().isEmpty();
        m_password->setEnabled(false);
        m_showPassword->setEnabled(false);
        m_status->setText(tr("Creating your share link…"));
        m_client->createSharedLink(m_asset.id, m_requestId, m_password->text());
    });
    connect(m_client, &ImmichClient::sharedLinkCreated, this, [this](const QUuid &id, const QUrl &url) {
        if (id != m_requestId)
            return;
        m_link->setText(url.toString(QUrl::FullyEncoded));
        m_copy->setEnabled(true);
        m_create->hide();
        m_password->setEnabled(true);
        m_password->setReadOnly(true);
        m_showPassword->setEnabled(true);
        m_status->setText(m_passwordProtected ? tr("Link ready. A password is required to view this media.")
            : tr("Link ready. Anyone with the link can view this media."));
    });
    connect(m_client, &ImmichClient::sharedLinkFailed, this, [this](const QUuid &id, const QString &message) {
        if (id != m_requestId)
            return;
        m_create->setEnabled(true);
        m_password->setEnabled(true);
        m_showPassword->setEnabled(true);
        m_status->setText(message);
    });
    connect(m_copy, &QPushButton::clicked, this, [this] {
        if (m_link->text().isEmpty())
            return;
        QApplication::clipboard()->setText(m_link->text());
        m_status->setText(m_passwordProtected ? tr("Link copied. Share the password separately.")
                                             : tr("Link copied to clipboard."));
    });
}
} // namespace Aurora
