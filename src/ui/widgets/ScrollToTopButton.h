#pragma once

#include <QPushButton>

class QGraphicsOpacityEffect;
class QPropertyAnimation;
class QScrollArea;

namespace Aurora {

// Round "back to top" button floating over a scroll area's bottom-right corner.
// It fades in once the area is scrolled a few screens down.
class ScrollToTopButton final : public QPushButton {
    Q_OBJECT

public:
    explicit ScrollToTopButton(QScrollArea *area);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateVisibility();
    void reposition();

    QScrollArea *m_area;
    QGraphicsOpacityEffect *m_opacity;
    QPropertyAnimation *m_fade;
    bool m_wanted = false;
};

} // namespace Aurora
