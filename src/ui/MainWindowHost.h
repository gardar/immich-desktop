#pragma once

#include <QObject>
#include <memory>

namespace Aurora {

class FolderSyncService;
class ImmichClient;
class Localization;
class MainWindow;
class ThemeManager;
class UpdateManager;

// Recreates constructor-built UI text after a language change, while keeping
// client connections, upload queues and folder watching alive outside the UI.
class MainWindowHost final : public QObject {
public:
    MainWindowHost(Localization *localization, ThemeManager *themeManager,
                   UpdateManager *updateManager, ImmichClient *client,
                   FolderSyncService *folderSync);
    ~MainWindowHost() override;

    MainWindow *window() const { return m_window.get(); }

private:
    void createWindow();
    void applyLanguage(const QString &language);

    Localization *m_localization;
    ThemeManager *m_themeManager;
    UpdateManager *m_updateManager;
    ImmichClient *m_client;
    FolderSyncService *m_folderSync;
    std::unique_ptr<MainWindow> m_window;
};

} // namespace Aurora
