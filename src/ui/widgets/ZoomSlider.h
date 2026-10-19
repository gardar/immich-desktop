#pragma once

#include <QWidget>

class QSlider;

namespace Aurora {

class ZoomSlider final : public QWidget {
    Q_OBJECT

public:
    explicit ZoomSlider(QWidget *parent = nullptr);

    void setValue(int size);
    void setCompact(bool compact);

signals:
    void valueChanged(int size);
    void stepRequested(int steps);

private:
    QSlider *m_slider;
};

} // namespace Aurora
