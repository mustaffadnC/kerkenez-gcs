#pragma once

#include <QWidget>

namespace kerkenez {

// Rotating compass rose with a fixed index at the top and a digital readout.
class CompassWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CompassWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override { return {140, 140}; }
    int heightForWidth(int w) const override { return w; }

public slots:
    void setHeading(float headingDeg);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float m_headingDeg = 0;
};

} // namespace kerkenez
