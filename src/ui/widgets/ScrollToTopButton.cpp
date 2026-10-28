#include "ui/widgets/ScrollToTopButton.h"

#include "ui/IconUtils.h"

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QScrollBar>

namespace Aurora {
namespace {

constexpr int kSize = 44;
constexpr int kMargin = 20;

} // namespace

ScrollToTopButton::ScrollToTopButton(QScrollArea *area)
    : QPushButton(area)
    , m_area(area)
    , m_opacity(new QGraphicsOpacityEffect(this))
    , m_fade(new QPropertyAnimation(m_opacity, "opacity", this))
{
    setObjectName(QStringLiteral("scrollToTopButton"));
    configureActionButton(this, QStringLiteral(":/icons/arrow-up.svg"));
    setActionButtonLabel(this, QString());
    setToolTip(tr("Back to top (Home)"));
    setAccessibleName(tr("Back to top"));
    setCursor(Qt::PointingHandCursor);
    setFixedSize(kSize, kSize);
    setFocusPolicy(Qt::NoFocus);
    setGraphicsEffect(m_opacity);
    m_opacity->setOpacity(0.0);
    m_fade->setDuration(180);
    hide();

    connect(m_fade, &QPropertyAnimation::finished, this, [this] {
        if (!m_wanted)
            hide();
    });
    connect(this, &QPushButton::clicked, this,
            [this] { m_area->verticalScrollBar()->setValue(0); });
    connect(m_area->verticalScrollBar(), &QScrollBar::valueChanged, this,
            &ScrollToTopButton::updateVisibility);
    m_area->viewport()->installEventFilter(this);
}

bool ScrollToTopButton::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_area->viewport() && event->type() == QEvent::Resize) {
        reposition();
        updateVisibility();
    }
    return QPushButton::eventFilter(watched, event);
}

void ScrollToTopButton::reposition()
{
    const QRect viewport = m_area->viewport()->geometry();
    move(viewport.right() - kSize - kMargin + 1, viewport.bottom() - kSize - kMargin + 1);
    raise();
}

void ScrollToTopButton::updateVisibility()
{
    // A couple of screens down is far enough that scrolling back is a chore.
    const bool wanted = m_area->verticalScrollBar()->value() > 2 * m_area->viewport()->height();
    if (wanted == m_wanted)
        return;
    m_wanted = wanted;
    if (wanted) {
        reposition();
        show();
    }
    m_fade->stop();
    m_fade->setStartValue(m_opacity->opacity());
    m_fade->setEndValue(wanted ? 1.0 : 0.0);
    m_fade->start();
}

} // namespace Aurora
