// Headless paint smoke tests: render every instrument into a QImage with
// normal and extreme values. Catches painter math crashes without a display.

#include <QImage>
#include <QTest>

#include "core/Vehicle.h"
#include "ui/AlertPanel.h"
#include "ui/CompassWidget.h"
#include "ui/PfdWidget.h"
#include "ui/StatusPanel.h"

using namespace kerkenez;

namespace {

QImage renderWidget(QWidget &widget, const QSize &size)
{
    widget.resize(size);
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    widget.render(&image);
    return image;
}

} // namespace

class TestWidgets : public QObject
{
    Q_OBJECT

private slots:
    void pfdRendersAcrossAttitudeRange()
    {
        PfdWidget pfd;
        const float attitudes[][3] = {
            {0, 0, 0}, {15, -5, 90}, {-45, 25, 359}, {180, 90, -180}, {-180, -90, 720},
        };
        for (const auto &att : attitudes) {
            pfd.setAttitude(att[0], att[1], att[2]);
            pfd.setSpeeds(12.5f, 14.0f, -2.5f, 55);
            pfd.setAltitudes(880.0f, 30.0f);
            const QImage image = renderWidget(pfd, {480, 360});
            QVERIFY(!image.isNull());
        }
    }

    void compassRendersFullCircle()
    {
        CompassWidget compass;
        for (float heading = -360; heading <= 720; heading += 45) {
            compass.setHeading(heading);
            const QImage image = renderWidget(compass, {170, 170});
            QVERIFY(!image.isNull());
        }
    }

    void panelsRenderWithLiveVehicle()
    {
        Vehicle vehicle;
        StatusPanel status(&vehicle);
        AlertPanel alerts(&vehicle);

        mavlink_message_t msg;
        mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, MAV_TYPE_QUADROTOR,
                                   MAV_AUTOPILOT_ARDUPILOTMEGA,
                                   MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_SAFETY_ARMED,
                                   4, MAV_STATE_ACTIVE);
        vehicle.handleMessage(msg);

        mavlink_msg_sys_status_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, 0, 0, 0, 500, 12100,
                                    1500, 15 /* battery low */, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        vehicle.handleMessage(msg);

        mavlink_msg_statustext_pack(1, MAV_COMP_ID_AUTOPILOT1, &msg, MAV_SEVERITY_ERROR,
                                    "EKF variance", 0, 0);
        vehicle.handleMessage(msg);

        QVERIFY(!renderWidget(status, {320, 160}).isNull());
        QVERIFY(!renderWidget(alerts, {320, 300}).isNull());
    }
};

QTEST_MAIN(TestWidgets)
#include "tst_widgets.moc"
