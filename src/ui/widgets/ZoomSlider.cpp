#include "ui/widgets/ZoomSlider.h"

#include "ui/GridZoom.h"
#include "ui/IconUtils.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QVariant>

namespace Aurora {

ZoomSlider::ZoomSlider(QWidget *parent)
    : QWidget(parent)
    , m_slider(new QSlider(Qt::Horizontal, this))
{
    auto *zoomOut = new QPushButton(this);
    auto *zoomIn = new QPushButton(this);
    for (QPushButton *button : {zoomOut, zoomIn}) {
        // Explicit QVariant: older Qt (e.g. 6.4 on Ubuntu CI) rejects bool here.
        button->setProperty("zoomButton", QVariant(true));
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
    }
    configureActionButton(zoomOut, QStringLiteral(":/icons/minus.svg"));
    configureActionButton(zoomIn, QStringLiteral(":/icons/plus.svg"));
    zoomOut->setToolTip(tr("Smaller thumbnails (Ctrl+−)"));
    zoomIn->setToolTip(tr("Larger thumbnails (Ctrl++)"));

    m_slider->setRange(GridZoom::kMinimum, GridZoom::kMaximum);
    m_slider->setFixedWidth(110);
    m_slider->setFocusPolicy(Qt::NoFocus);
    m_slider->setToolTip(tr("Thumbnail size (Ctrl+scroll)"));
    m_slider->setAccessibleName(tr("Thumbnail size"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(zoomOut);
    layout->addWidget(m_slider);
    layout->addWidget(zoomIn);

    connect(zoomOut, &QPushButton::clicked, this, [this] { emit stepRequested(-1); });
    connect(zoomIn, &QPushButton::clicked, this, [this] { emit stepRequested(1); });
    connect(m_slider, &QSlider::valueChanged, this, &ZoomSlider::valueChanged);
}

void ZoomSlider::setValue(int size)
{
    const QSignalBlocker blocker(m_slider);
    m_slider->setValue(size);
}

void ZoomSlider::setCompact(bool compact)
{
    m_slider->setVisible(!compact);
}

} // namespace Aurora
