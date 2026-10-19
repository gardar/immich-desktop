#pragma once

#include <QWidget>

class QButtonGroup;
class QLabel;
class QVariantAnimation;
class QVBoxLayout;

namespace Aurora {

class AnimatedButton;
class ThemeManager;

class Sidebar final : public QWidget {
    Q_OBJECT

public:
    explicit Sidebar(ThemeManager *themeManager, QWidget *parent = nullptr);
    void setCurrentPage(int index);
    void toggleCollapsed();
    void setNarrow(bool narrow);

signals:
    void pageRequested(int index);

private:
    void addNavigation(const QString &label, const QString &icon, int index,
                       bool checkable = true);
    void clearNavigation();
    void showMainNavigation();
    void showSettingsNavigation();
    bool isCollapsed() const;
    void refreshLayout(bool animate);

    ThemeManager *m_themeManager;
    QLabel *m_navigationLabel;
    QVBoxLayout *m_navigationLayout;
    QButtonGroup *m_navigationGroup;
    bool m_settingsMode = false;
    QLabel *m_version;
    AnimatedButton *m_toggleButton;
    QVariantAnimation *m_widthAnimation;
    bool m_collapsed = false;
    bool m_narrow = false;
    bool m_expandedWhileNarrow = false;
};

} // namespace Aurora
