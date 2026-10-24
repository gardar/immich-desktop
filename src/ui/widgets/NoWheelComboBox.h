#pragma once

#include <QComboBox>
#include <QWheelEvent>

namespace Aurora {

// QComboBox changes its value on mouse wheel by default, which is easy to do
// accidentally while scrolling a settings page. Always ignore the wheel on the
// combo itself (the open popup list still scrolls on its own); ignored events
// reach the parent scroll area.
class NoWheelComboBox : public QComboBox {
public:
    using QComboBox::QComboBox;

protected:
    void wheelEvent(QWheelEvent *event) override { event->ignore(); }
};

} // namespace Aurora
