#include "ui/MainWindowHost.h"

#include "core/Localization.h"
#include "ui/MainWindow.h"

namespace Aurora {

MainWindowHost::MainWindowHost(Localization *localization, ThemeManager *themeManager,
                               UpdateManager *updateManager, ImmichClient *client,
                               FolderSyncService *folderSync)
    : m_localization(localization)
    , m_themeManager(themeManager)
    , m_updateManager(updateManager)
    , m_client(client)
    , m_folderSync(folderSync)
{
    createWindow();
}

MainWindowHost::~MainWindowHost() = default;

void MainWindowHost::createWindow()
{
    m_window = std::make_unique<MainWindow>(m_themeManager, m_updateManager,
                                          m_client, m_folderSync);
    // Wait until the combobox's signal delivery finishes before deleting its UI.
    connect(m_window.get(), &MainWindow::languageChangeRequested, this,
            [this](const QString &language) { applyLanguage(language); }, Qt::QueuedConnection);
}

void MainWindowHost::applyLanguage(const QString &language)
{
    const QByteArray geometry = m_window->saveGeometry();
    const bool visible = m_window->isVisible();
    m_window->hide();
    m_window.reset();
    m_localization->initialize(language);
    createWindow();
    m_window->restoreGeometry(geometry);
    m_window->showAppearanceSettings();
    if (visible)
        m_window->show();
}

} // namespace Aurora
