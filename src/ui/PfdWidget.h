#pragma once

#include <QWidget>

namespace kerkenez {

// Primary flight display: artificial horizon with pitch ladder and roll scale,
// speed tape (left) and altitude tape (right). Pure QPainter, no assets.
class PfdWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PfdWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override { return {360, 280}; }

public slots:
    void setAttitude(float rollDeg, float pitchDeg, float yawDeg);
    void setSpeeds(float airspeed, float groundspeed, float climbRate, int throttlePct);
    void setAltitudes(float altMsl, float altRel);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawHorizonAndLadder(QPainter &p, const QRectF &r) const;
    void drawRollScale(QPainter &p, const QRectF &r) const;
    void drawAircraftSymbol(QPainter &p, const QRectF &r) const;
    void drawSpeedTape(QPainter &p, const QRectF &r) const;
    void drawAltitudeTape(QPainter &p, const QRectF &r) const;

    float m_rollDeg = 0;
    float m_pitchDeg = 0;
    float m_yawDeg = 0;
    float m_airspeed = 0;
    float m_groundspeed = 0;
    float m_climbRate = 0;
    int m_throttlePct = 0;
    float m_altMsl = 0;
    float m_altRel = 0;
};

} // namespace kerkenez
