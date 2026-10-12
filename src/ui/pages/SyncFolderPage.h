#pragma once

#include <QWidget>
#include <QList>
#include <QStringList>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace Aurora {

class FolderSyncService;
class ThemeManager;

class SyncFolderPage final : public QWidget {
    Q_OBJECT

public:
    explicit SyncFolderPage(FolderSyncService *syncService, ThemeManager *themeManager,
                            QWidget *parent = nullptr);

private slots:
    void addFolder();
    void saveSettings();
    void refreshStatus();

private:
    void addFolderRow(const QString &path);
    void loadFolderRows(const QStringList &paths);
    QStringList selectedFolders() const;
    void chooseFolder(QLineEdit *field);
    FolderSyncService *m_syncService = nullptr;
    ThemeManager *m_themeManager = nullptr;
    QCheckBox *m_enabled = nullptr;
    QCheckBox *m_localNetworkOnly = nullptr;
    QList<QLineEdit *> m_folderPaths;
    QVBoxLayout *m_foldersLayout = nullptr;
    QPushButton *m_addFolderButton = nullptr;
    QPushButton *m_scanButton = nullptr;
    QLabel *m_statusIcon = nullptr;
    QLabel *m_statusText = nullptr;
};

} // namespace Aurora
