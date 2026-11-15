#include "trayicon.h"
#include "device.h"
#include "devicescanner.h"
#include "iconprovider.h"
#include "settings.h"

#include <QActionGroup>
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QStyleHints>

#ifdef Q_OS_WIN
#include <QSettings>
#endif

TrayIcon::TrayIcon(Device *device, Settings *settings, DeviceScanner *scanner, QObject *parent)
    : QObject(parent), m_device(device), m_settings(settings), m_scanner(scanner) {
    m_tray.setContextMenu(&m_menu);
    connect(&m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            emit toggleRequested();
    });
    connect(m_device, &Device::stateChanged, this, &TrayIcon::refresh);
    connect(m_device, &Device::batteryChanged, this, &TrayIcon::refresh);
    connect(m_device, &Device::ancChanged, this, &TrayIcon::refresh);
    connect(m_device, &Device::modelChanged, this, &TrayIcon::refresh);
    connect(m_settings, &Settings::changed, this, &TrayIcon::refresh);
    connect(m_scanner, &DeviceScanner::adapterChanged, this, &TrayIcon::refresh);
    connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged, this, &TrayIcon::refresh);
    refresh();
    m_tray.show();
}

bool TrayIcon::lightTaskbar() {
#ifdef Q_OS_WIN
    QSettings personalize("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                          QSettings::NativeFormat);
    return personalize.value("SystemUsesLightTheme", 0).toInt() == 1;
#else
    // Omarchy's bar and most Linux panels are dark; the icon is white there.
    return false;
#endif
}

// A 3x5 dot-matrix digit set: small enough that "100" still fits a 22 px
// tray slot with 2 px dots, in the spirit of Nothing's dot typography.
static const char *const Digits[10][5] = {
    {"111", "101", "101", "101", "111"}, {"010", "110", "010", "010", "111"},
    {"111", "001", "111", "100", "111"}, {"111", "001", "111", "001", "111"},
    {"101", "101", "111", "001", "001"}, {"111", "100", "111", "001", "111"},
    {"111", "100", "111", "101", "111"}, {"111", "001", "001", "001", "001"},
    {"111", "101", "111", "101", "111"}, {"111", "101", "111", "001", "111"},
};

int TrayIcon::roundedLevel(int battery) { return qBound(0, qRound(battery / 5.0) * 5, 100); }

static void drawNumber(QPainter &p, int value, int size, const QColor &color) {
    const QString text = QString::number(value);
    const int cols = int(text.size()) * 4 - 1, rows = 5;
    // Whole pixels per dot, so the digits stay crisp at every tray size.
    const int pitch = qMax(1, qMin(size / cols, size / rows));
    const int dot = pitch >= 3 ? pitch - qMax(1, pitch / 4) : pitch;
    const int width = cols * pitch - (pitch - dot), height = rows * pitch - (pitch - dot);
    const int x0 = (size - width) / 2, y0 = (size - height) / 2;
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    for (int i = 0; i < text.size(); ++i) {
        const auto &glyph = Digits[text[i].digitValue()];
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < 3; ++c) {
                if (glyph[r][c] != '1')
                    continue;
                const QRectF cell(x0 + (i * 4 + c) * pitch, y0 + r * pitch, dot, dot);
                if (dot >= 4)
                    p.drawEllipse(cell);
                else
                    p.fillRect(cell, color);
            }
    }
}

QImage TrayIcon::renderImage(Look look, int battery, bool lightTaskbar, int size) {
    const QColor fg = lightTaskbar ? QColor("#000000") : QColor("#FFFFFF");
    if (look == Battery && battery >= 0) {
        QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        drawNumber(p, roundedLevel(battery), size, battery <= 20 ? QColor("#D71921") : fg);
        return image;
    }
    QImage image = IconProvider::render("tray", fg, QSize(size, size));
    if (look == Disconnected) {
        // The earbuds glyph, faded and struck through: the universal "off".
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.fillRect(image.rect(), QColor(0, 0, 0, 120));
        const QLineF slash(size * 0.16, size * 0.10, size * 0.84, size * 0.90);
        p.setCompositionMode(QPainter::CompositionMode_Clear);
        p.setPen(QPen(Qt::black, qMax(3.0, size * 0.22), Qt::SolidLine, Qt::RoundCap));
        p.drawLine(slash);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.setPen(QPen(fg, qMax(1.5, size * 0.09), Qt::SolidLine, Qt::RoundCap));
        p.drawLine(slash);
    }
    return image;
}

QIcon TrayIcon::renderIcon(Look look, int battery, bool lightTaskbar) {
    QIcon icon;
    for (int size : {16, 20, 22, 24, 32, 44, 64})
        icon.addPixmap(QPixmap::fromImage(renderImage(look, battery, lightTaskbar, size)));
    return icon;
}

void TrayIcon::notify(const QString &title, const QString &message) {
    m_tray.showMessage(title, message, QIcon(":/icons/app.svg"), 6000);
}

void TrayIcon::refresh() {
    const bool connected = m_device->state() == Device::Connected;
    const int left = m_device->batteryLeft(), right = m_device->batteryRight();
    int lowest = -1;
    for (int v : {left, right})
        if (v >= 0)
            lowest = lowest < 0 ? v : qMin(lowest, v);
    // The icon shows the earbud that will run out first; the case only when
    // no earbud has reported yet.
    const int shown = lowest >= 0 ? lowest : m_device->batteryCase();

    Look look = Disconnected;
    if (connected)
        look = m_settings->batteryInTray() && shown >= 0 ? Battery : Earbuds;
    else if (m_device->state() == Device::Connecting)
        look = Earbuds;
    m_tray.setIcon(renderIcon(look, shown, lightTaskbar()));

    QStringList lines{QStringLiteral("Nothing X")};
    if (connected) {
        lines << m_device->displayName();
        QStringList parts;
        auto fmt = [](const QString &label, int v) { return v >= 0 ? QStringLiteral("%1 %2%").arg(label).arg(v) : QString(); };
        for (const auto &s : {fmt(tr("L"), left), fmt(tr("R"), right), fmt(tr("Case"), m_device->batteryCase())})
            if (!s.isEmpty())
                parts << s;
        if (!parts.isEmpty())
            lines << parts.join(QStringLiteral(" · "));
    } else {
        lines << (m_device->state() == Device::Connecting ? tr("Connecting…")
                  : !m_scanner->adapterPoweredOn()   ? tr("Bluetooth is off")
                                                     : tr("Not connected"));
    }
    m_tray.setToolTip(lines.join('\n'));

    // Low battery warning, once per drop below 20%, then again below 10%.
    if (connected && lowest >= 0 && m_settings->lowBatteryNotification()) {
        const int threshold = lowest <= 10 ? 10 : lowest <= 20 ? 20 : 101;
        if (threshold < m_lastLowNotified) {
            notify(tr("Earbuds battery low"), tr("%1 is at %2%").arg(m_device->displayName()).arg(lowest));
            m_lastLowNotified = threshold;
        } else if (threshold == 101) {
            m_lastLowNotified = 101;
        }
    }
    rebuildMenu();
}

void TrayIcon::rebuildMenu() {
    m_menu.clear();
    m_menu.addAction(tr("Open Nothing X"), this, &TrayIcon::showRequested);
    m_menu.addSeparator();

    const bool connected = m_device->state() == Device::Connected;
    if (connected && m_device->has("anc")) {
        auto *group = new QActionGroup(&m_menu);
        const int current = m_device->ancMode();
        const bool cancelling = current >= Proto::AncHigh && current <= Proto::AncAdaptive;
        auto add = [&](const QString &label, bool checked, auto apply) {
            QAction *a = m_menu.addAction(label);
            a->setCheckable(true);
            a->setChecked(checked);
            group->addAction(a);
            connect(a, &QAction::triggered, this, apply);
        };
        add(tr("Noise Cancellation"), cancelling, [this] { m_device->setNoiseCancellation(); });
        if (m_device->has("transparency"))
            add(tr("Transparency"), current == Proto::AncTransparency, [this] { m_device->setAncMode(Proto::AncTransparency); });
        add(tr("Off"), current == Proto::AncOff, [this] { m_device->setAncMode(Proto::AncOff); });
        m_menu.addSeparator();
    }

    if (connected)
        m_menu.addAction(tr("Disconnect"), m_device, &Device::disconnectFromDevice);
    else if (!m_settings->lastDeviceAddress().isEmpty())
        m_menu.addAction(tr("Connect to %1").arg(m_settings->lastDeviceName()), m_device, &Device::connectToLast);
    m_menu.addSeparator();
    m_menu.addAction(tr("Quit"), this, &TrayIcon::quitRequested);
}
