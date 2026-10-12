#include "ui/pages/SyncFolderPage.h"

#include "core/FolderSyncService.h"
#include "core/ThemeManager.h"
#include "ui/IconUtils.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QUrl>
#include <QVBoxLayout>

namespace Aurora {

SyncFolderPage::SyncFolderPage(FolderSyncService *syncService, ThemeManager *themeManager,
                               QWidget *parent)
    : QWidget(parent)
    , m_syncService(syncService)
    , m_themeManager(themeManager)
    , m_enabled(new QCheckBox(tr("Watch folders and upload new media"), this))
    , m_localNetworkOnly(new QCheckBox(
          tr("Upload only when Immich is available on the local network"), this))
    , m_addFolderButton(new QPushButton(this))
    , m_scanButton(new QPushButton(tr("Scan now"), this))
    , m_statusIcon(new QLabel(this))
    , m_statusText(new QLabel(this))
{
    auto *root = new QVBoxLayout(this);
    configureActionButton(m_addFolderButton, QStringLiteral(":/ui/plus.svg"));
    m_addFolderButton->setObjectName(QStringLiteral("addSyncFolderButton"));
    m_addFolderButton->setToolTip(tr("Add another folder"));
    m_addFolderButton->setAccessibleName(tr("Add another folder"));
    m_addFolderButton->setCursor(Qt::PointingHandCursor);
    configureActionButton(m_scanButton, QStringLiteral(":/icons/refresh-cw.svg"));
    root->setContentsMargins(0, 12, 0, 0);
    root->setSpacing(14);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 8, 0);

    auto *card = new QFrame(this);
    card->setProperty("card", true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 22);
    cardLayout->setSpacing(14);

    auto *title = new QLabel(tr("Folder sync"), card);
    title->setProperty("section", true);
    auto *description = new QLabel(
        tr("Choose folders on this computer. Photos and videos dropped into them "
           "(including subfolders) are uploaded to Immich automatically on Windows, "
           "macOS, and Linux. Files stay on disk after upload."),
        card);
    description->setProperty("subheading", true);
    description->setWordWrap(true);

    const FolderSyncSettings current = m_syncService->settings();
    m_enabled->setChecked(current.enabled);
    m_enabled->setCursor(Qt::PointingHandCursor);
    m_localNetworkOnly->setChecked(current.localNetworkOnly);
    m_localNetworkOnly->setCursor(Qt::PointingHandCursor);
    m_localNetworkOnly->setToolTip(
        tr("Uses your Local URL from Immich Server settings. Files still appear in "
           "Library with an unsaved badge until the local server is reachable."));
    auto *foldersHeader = new QHBoxLayout;
    auto *foldersLabel = new QLabel(tr("Folders"), card);
    foldersLabel->setProperty("section", true);
    foldersHeader->addWidget(foldersLabel);
    foldersHeader->addStretch();
    foldersHeader->addWidget(m_addFolderButton);
    m_foldersLayout = new QVBoxLayout;
    m_foldersLayout->setSpacing(8);
    loadFolderRows(current.folderPaths);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(8);
    actions->addWidget(m_scanButton);
    actions->addStretch();

    auto *statusRow = new QHBoxLayout;
    statusRow->setSpacing(10);
    m_statusIcon->setFixedSize(28, 28);
    m_statusIcon->setAlignment(Qt::AlignCenter);
    m_statusText->setProperty("subheading", true);
    m_statusText->setWordWrap(true);
    statusRow->addWidget(m_statusIcon, 0, Qt::AlignTop);
    statusRow->addWidget(m_statusText, 1);

    auto *legend = new QLabel(
        tr("Status icons: saved · saving · unsaved · error saving"), card);
    legend->setProperty("subheading", true);
    legend->setWordWrap(true);

    auto *legendIcons = new QHBoxLayout;
    legendIcons->setSpacing(16);
    const auto addLegend = [this, legendIcons](const QString &path, const QString &label) {
        auto *icon = new QLabel(this);
        icon->setFixedSize(22, 22);
        icon->setPixmap(renderSvgIcon(path, m_themeManager->palette().mutedText, QSize(20, 20),
                                      devicePixelRatioF()));
        auto *text = new QLabel(label, this);
        text->setProperty("subheading", true);
        auto *row = new QHBoxLayout;
        row->setSpacing(6);
        row->addWidget(icon);
        row->addWidget(text);
        legendIcons->addLayout(row);
    };
    addLegend(QStringLiteral(":/icons/cloud-check.svg"), tr("Saved"));
    addLegend(QStringLiteral(":/icons/cloud-upload.svg"), tr("Saving"));
    addLegend(QStringLiteral(":/icons/cloud-off.svg"), tr("Unsaved"));
    addLegend(QStringLiteral(":/icons/cloud-alert.svg"), tr("Error"));
    legendIcons->addStretch();

    cardLayout->addWidget(title);
    cardLayout->addWidget(description);
    cardLayout->addWidget(m_enabled);
    cardLayout->addWidget(m_localNetworkOnly);
    cardLayout->addLayout(foldersHeader);
    cardLayout->addLayout(m_foldersLayout);
    cardLayout->addLayout(actions);
    cardLayout->addLayout(statusRow);
    cardLayout->addWidget(legend);
    cardLayout->addLayout(legendIcons);
    contentLayout->addWidget(card);
    contentLayout->addStretch();
    scroll->setWidget(content);
    root->addWidget(scroll);

    connect(m_enabled, &QCheckBox::toggled, this, &SyncFolderPage::saveSettings);
    connect(m_localNetworkOnly, &QCheckBox::toggled, this, &SyncFolderPage::saveSettings);
    connect(m_addFolderButton, &QPushButton::clicked, this, &SyncFolderPage::addFolder);
    connect(m_scanButton, &QPushButton::clicked, this, [this] {
        saveSettings();
        m_syncService->scanNow();
    });
    connect(m_syncService, &FolderSyncService::statusChanged, this,
            &SyncFolderPage::refreshStatus);
    connect(m_syncService, &FolderSyncService::settingsChanged, this, [this](const FolderSyncSettings &settings) {
        const QSignalBlocker enabledBlocker(m_enabled);
        const QSignalBlocker localOnlyBlocker(m_localNetworkOnly);
        m_enabled->setChecked(settings.enabled);
        m_localNetworkOnly->setChecked(settings.localNetworkOnly);
        if (selectedFolders() != settings.folderPaths)
            loadFolderRows(settings.folderPaths);
        refreshStatus();
    });
    connect(m_themeManager, &ThemeManager::appearanceChanged, this,
            &SyncFolderPage::refreshStatus);

    refreshStatus();
}

void SyncFolderPage::chooseFolder(QLineEdit *field)
{
    const QString folder = QFileDialog::getExistingDirectory(
        this, tr("Choose sync folder"), field->text().trimmed(), QFileDialog::ShowDirsOnly);
    if (folder.isEmpty())
        return;
    field->setText(folder);
    saveSettings();
}

void SyncFolderPage::saveSettings()
{
    FolderSyncSettings settings;
    settings.enabled = m_enabled->isChecked();
    settings.localNetworkOnly = m_localNetworkOnly->isChecked();
    settings.folderPaths = selectedFolders();
    m_syncService->setSettings(settings);
}

QStringList SyncFolderPage::selectedFolders() const
{
    QStringList paths;
    for (auto *field : m_folderPaths) {
        const QString path = field->text().trimmed();
        if (!path.isEmpty())
            paths.append(path);
    }
    return paths;
}

void SyncFolderPage::addFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(this, tr("Add sync folder"),
        selectedFolders().value(0), QFileDialog::ShowDirsOnly);
    if (folder.isEmpty())
        return;
    if (m_folderPaths.size() == 1 && m_folderPaths.first()->text().trimmed().isEmpty())
        m_folderPaths.first()->setText(folder);
    else
        addFolderRow(folder);
    saveSettings();
}

void SyncFolderPage::loadFolderRows(const QStringList &paths)
{
    m_folderPaths.clear();
    while (auto *item = m_foldersLayout->takeAt(0)) {
        if (auto *widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
    if (paths.isEmpty())
        addFolderRow(QString());
    else
        for (const auto &path : paths)
            addFolderRow(path);
}

void SyncFolderPage::addFolderRow(const QString &path)
{
    auto *row = new QWidget(this);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    auto *field = new QLineEdit(path, row);
    field->setObjectName(QStringLiteral("syncFolderPath"));
    field->setAccessibleName(tr("Sync folder path"));
    field->setPlaceholderText(tr("Choose a folder to sync…"));
    field->setClearButtonEnabled(true);
    m_folderPaths.append(field);
    layout->addWidget(field, 1);
    auto *browse = new QPushButton(tr("Browse…"), row);
    auto *open = new QPushButton(tr("Open folder"), row);
    auto *remove = new QPushButton(row);
    remove->setObjectName(QStringLiteral("removeSyncFolderButton"));
    configureActionButton(browse, QStringLiteral(":/icons/folder.svg"));
    configureActionButton(open, QStringLiteral(":/icons/external-link.svg"));
    configureActionButton(remove, QStringLiteral(":/icons/trash.svg"));
    remove->setToolTip(tr("Remove folder from sync"));
    remove->setAccessibleName(tr("Remove folder from sync"));
    open->setEnabled(!path.trimmed().isEmpty());
    for (auto *button : {browse, open, remove}) {
        button->setCursor(Qt::PointingHandCursor);
        layout->addWidget(button);
    }
    m_foldersLayout->addWidget(row);
    connect(field, &QLineEdit::editingFinished, this, &SyncFolderPage::saveSettings);
    connect(field, &QLineEdit::textChanged, this, [this, open](const QString &text) {
        open->setEnabled(!text.trimmed().isEmpty());
        refreshStatus();
    });
    connect(browse, &QPushButton::clicked, this, [this, field] { chooseFolder(field); });
    connect(open, &QPushButton::clicked, this, [field] {
        if (!field->text().trimmed().isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(field->text().trimmed()));
    });
    connect(remove, &QPushButton::clicked, this, [this, row, field] {
        m_folderPaths.removeOne(field);
        m_foldersLayout->removeWidget(row);
        row->hide();
        row->deleteLater();
        if (m_folderPaths.isEmpty())
            addFolderRow(QString());
        saveSettings();
    });
}

void SyncFolderPage::refreshStatus()
{
    const QColor color = m_themeManager->palette().text;
    m_statusIcon->setPixmap(renderSvgIcon(m_syncService->statusIconPath(), color,
                                          QSize(24, 24), devicePixelRatioF()));
    m_statusText->setText(m_syncService->statusMessage());
    m_scanButton->setEnabled(m_enabled->isChecked() &&
                             !selectedFolders().isEmpty());
}

} // namespace Aurora
