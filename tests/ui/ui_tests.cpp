// Clicks through the real popup on the simulated Ear (3), the way a user
// would, and checks what the earbuds end up with.
#include "app.h"
#include "device.h"
#include <QQmlContext>
#include <QQmlExpression>
#include <QtQml>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <cstdio>

static QQuickWindow *win() {
    for (QWindow *w : QGuiApplication::topLevelWindows())
        if (auto *q = qobject_cast<QQuickWindow *>(w); q && q->title() == "Nothing X")
            return q;
    return nullptr;
}
static void collect(QQuickItem *item, const QString &text, QList<QQuickItem *> &out) {
    if (!item || !item->isVisible() || item->opacity() == 0)
        return;
    if (item->property("text").toString() == text)
        out << item;
    for (QQuickItem *c : item->childItems())
        collect(c, text, out);
}
static bool click(const QString &text, int index = 0) {
    // Let page transitions and layout settle before aiming.
    QTest::qWait(350);
    QList<QQuickItem *> found;
    collect(win()->contentItem(), text, found);
    if (found.size() <= index) { printf("  !! no visible '%s'\n", qPrintable(text)); return false; }
    QQuickItem *it = found[index];
    // Scroll the page so the item is on screen, like a user would.
    for (QQuickItem *p = it->parentItem(); p; p = p->parentItem()) {
        if (p->inherits("QQuickFlickable")) {
            QQuickItem *content = p->property("contentItem").value<QQuickItem *>();
            const qreal y = it->mapToItem(content, QPointF(0, 0)).y();
            const qreal maxY = qMax<qreal>(0, p->property("contentHeight").toReal() - p->height());
            p->setProperty("contentY", qBound<qreal>(0, y - p->height() / 3, maxY));
            QTest::qWait(600);
            break;
        }
    }
    const QPointF p = it->mapToScene(QPointF(it->width() / 2, it->height() / 2));
    QTest::mouseClick(win(), Qt::LeftButton, {}, p.toPoint());
    QTest::qWait(250);
    return true;
}
static int fails = 0;
static void check(const char *what, bool ok) { printf("%s %s\n", ok ? "ok  " : "FAIL", what); if (!ok) ++fails; }

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication a(argc, argv);
    a.setOrganizationName("nothingx-uitest");
    a.setApplicationName("nothingx-uitest");
    QFile::remove(QDir::tempPath() + "/nothingx-demo.conf");
    App::Options o; o.demo = true; o.hidden = true;
    App app(o);
    if (!app.start()) return 2;
    win()->show();
    QTest::qWait(1500);
    printf("popup ready\n");
    // Read the device through the popup's own QML context.
    auto eval = [&](const QString &expr) {
        QQmlExpression e(qmlContext(win()), win(), expr);
        return e.evaluate();
    };
    check("connected", eval("device.state").toInt() == 2);
    click("Transparency");
    check("noise control -> Transparency", eval("device.ancMode").toInt() == 7);
    click("Off");
    check("noise control -> Off", eval("device.ancMode").toInt() == 5);
    click("Noise Cancellation");
    check("noise cancellation restores the last level (Adaptive)", eval("device.ancMode").toInt() == 4);
    click("Mid");
    check("ANC level Mid", eval("device.ancMode").toInt() == 2);

    click("Fixed");
    // spatial with Ultra Bass on asks first (spaceOrAdvance)
    click("Confirm");
    check("spatial fixed after confirming", eval("device.values.spatial").toInt() == 1);
    check("bass turned off by spatial", eval("device.values.bassOn").toBool() == false);
    click("Off", 1);
    check("spatial off", eval("device.values.spatial").toInt() == 0);

    click("Equaliser");
    click("More bass");
    check("preset More bass", eval("device.values.eqPreset").toInt() == 3);
    click("Custom");
    check("preset Custom", eval("device.values.eqPreset").toInt() == 5);
    click("Advanced");
    check("advanced EQ on", eval("device.values.advancedEq").toBool());
    click("New profile");
    check("profile saved", eval("settings.deviceValue(device.address, 'eqProfiles', []).length").toInt() >= 1);
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);

    click("Controls");
    click("Double pinch");
    click("Voice assistant");
    check("left double pinch -> voice assistant", eval("device.gestureAction(2, 2)").toInt() == 11);
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);
    click("Pinch and hold");
    click("Off");
    check("hold cycle now includes Off (10)", eval("device.gestureAction(2, 7)").toInt() == 10);
    check("right earbud follows the cycle", eval("device.gestureAction(3, 7)").toInt() == 10);
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);
    click("Talk button");
    click("Press and hold");
    click("Desktop assistant");
    check("TALK button -> Desktop assistant (Essential Space, 33)", eval("device.gestureAction(4, 7)").toInt() == 33);
    if (!qEnvironmentVariableIsEmpty("UI_SHOT")) win()->grabWindow().save(qEnvironmentVariable("UI_SHOT"));
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);

    click("Device Settings");
    click("Low-latency mode");
    check("low latency on", eval("device.values.lowLag").toBool());
    click("High-quality audio");
    click("LDAC");
    click("Confirm");
    check("LDAC on", eval("device.values.ldac").toBool());
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);
    click("Dual Connection");
    click("Disconnect");
    click("Confirm");
    QTest::qWait(600);
    check("phone disconnected from dual list", eval("device.pairedDevices[1].connected").toBool() == false);
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);

    click("Super Mic");
    click("Walkie-talkie mode");
    check("walkie-talkie on", eval("device.values.walkieTalkie").toBool());
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);

    click("Ear tip fit test");
    click("Start test");
    check("fit test running", eval("device.values.fitRunning").toBool());
    QTest::qWait(7000);
    check("fit test result: left good, right adjust", eval("device.values.fitLeft").toInt() == 0 && eval("device.values.fitRight").toInt() == 1);
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);

    click("Device Settings");
    click("Device details");
    check("serial number shown", eval("device.values.serial").toString() == "SH10DEM0000173");
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);
    QTest::keyClick(win(), Qt::Key_Escape); QTest::qWait(300);

    printf("\n%d failed\n", fails);
    return fails ? 1 : 0;
}
