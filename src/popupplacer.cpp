#include "popupplacer.h"

#include <QCursor>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QQuickWindow>
#include <QRasterWindow>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QTimer>
#include <functional>

#ifdef HAVE_LAYERSHELLQT
#include <LayerShellQt/Window>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

static constexpr int Margin = 10;

namespace {
class ClickCatcher : public QRasterWindow {
public:
    std::function<void()> onClick;

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(QRect(QPoint(), size()), Qt::transparent);
    }
    void mousePressEvent(QMouseEvent *) override {
        if (onClick)
            onClick();
    }
};
} // namespace

PopupPlacer::PopupPlacer(QQuickWindow *window, QSystemTrayIcon *tray, QObject *parent)
    : QObject(parent), m_window(window), m_tray(tray) {
    m_window->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_window->installEventFilter(this);
    connect(m_window, &QWindow::activeChanged, this, &PopupPlacer::dismissIfInactive);
    if (QGuiApplication::platformName().startsWith("wayland"))
        setupLayerShell();

#ifdef Q_OS_WIN
    // Windows 11 rounds the corners of a borderless window only when asked.
    m_window->create();
    const DWORD round = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(reinterpret_cast<HWND>(m_window->winId()), 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */,
                          &round, sizeof(round));
#endif
}

QString PopupPlacer::omarchyTrayCorner(const QString &shellJson) {
    QFile file(shellJson);
    if (!file.open(QIODevice::ReadOnly))
        return QStringLiteral("top-right");
    const QJsonObject bar = QJsonDocument::fromJson(file.readAll()).object().value("bar").toObject();
    const QString position = bar.value("position").toString("top");
    const QJsonObject layout = bar.value("layout").toObject();

    // Find which section holds the tray, to know which end of the bar it is on.
    QString section = "right";
    for (auto it = layout.begin(); it != layout.end(); ++it)
        for (const auto &widget : it.value().toArray())
            if (widget.toObject().value("id").toString() == "omarchy.tray")
                section = it.key();
    const bool start = section.startsWith("left") || section == "top";

    if (position == "bottom")
        return start ? "bottom-left" : "bottom-right";
    if (position == "left")
        return start ? "top-left" : "bottom-left";
    if (position == "right")
        return start ? "top-right" : "bottom-right";
    return start ? "top-left" : "top-right";
}

void PopupPlacer::setupLayerShell() {
#ifdef HAVE_LAYERSHELLQT
    auto *ls = LayerShellQt::Window::get(m_window);
    if (!ls)
        return;
    const QString corner = omarchyTrayCorner(QDir::homePath() + "/.config/omarchy/shell.json");
    LayerShellQt::Window::Anchors anchors;
    anchors |= corner.startsWith("top") ? LayerShellQt::Window::AnchorTop : LayerShellQt::Window::AnchorBottom;
    anchors |= corner.endsWith("right") ? LayerShellQt::Window::AnchorRight : LayerShellQt::Window::AnchorLeft;
    ls->setAnchors(anchors);
    ls->setMargins(QMargins(Margin, Margin, Margin, Margin));
    ls->setLayer(LayerShellQt::Window::LayerTop);
    ls->setExclusiveZone(0);
    // On demand, so clicking anywhere else takes the focus away and the
    // popup closes like a flyout.
    ls->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
    ls->setScope(QStringLiteral("nothingx"));
    m_layerShell = true;

    auto *catcher = new ClickCatcher;
    QSurfaceFormat format = catcher->format();
    format.setAlphaBufferSize(8);
    catcher->setFormat(format);
    catcher->onClick = [this] { hide(); };
    auto *cls = LayerShellQt::Window::get(catcher);
    cls->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorBottom |
                                                  LayerShellQt::Window::AnchorLeft | LayerShellQt::Window::AnchorRight));
    cls->setLayer(LayerShellQt::Window::LayerTop);
    cls->setExclusiveZone(0); // keeps clear of the bar, so the tray icon still toggles
    cls->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    cls->setScope(QStringLiteral("nothingx-dismiss"));
    m_clickCatcher = catcher;
#endif
}

QPoint PopupPlacer::dockedPosition(const QRect &screen, const QRect &available, const QRect &trayIcon,
                                   const QSize &popup, int margin) {
    // The taskbar is on the side where the available area is cut short.
    const bool top = available.top() > screen.top();
    const bool left = available.left() > screen.left();
    const bool right = available.x() + available.width() < screen.x() + screen.width();
    // Exclusive edges: QRect::right()/bottom() are off by one for this.
    const int minX = available.x() + margin, maxX = available.x() + available.width() - popup.width() - margin;
    const int minY = available.y() + margin, maxY = available.y() + available.height() - popup.height() - margin;
    const bool iconKnown = trayIcon.isValid() && !trayIcon.isEmpty();

    int x = left ? minX : maxX;
    int y = top ? minY : maxY;
    if (iconKnown) {
        if (left || right)
            y = trayIcon.center().y() - popup.height() / 2;  // vertical taskbar
        else
            x = trayIcon.center().x() - popup.width() / 2;   // horizontal taskbar
    }
    return {qBound(minX, x, qMax(minX, maxX)), qBound(minY, y, qMax(minY, maxY))};
}

void PopupPlacer::placeFloating() {
    const QRect icon = m_tray ? m_tray->geometry() : QRect();
    QScreen *screen = nullptr;
    if (icon.isValid())
        screen = QGuiApplication::screenAt(icon.center());
    if (!screen)
        screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    m_window->setScreen(screen);
    m_window->setPosition(dockedPosition(screen->geometry(), screen->availableGeometry(), icon,
                                         m_window->size(), Margin));
}

void PopupPlacer::show() {
    if (!m_layerShell)
        placeFloating();
    // Mapped first so the popup stacks above it.
    if (m_clickCatcher && !m_clickCatcher->isVisible())
        m_clickCatcher->show();
    m_window->show();
    m_window->raise();
    m_window->requestActivate();
    m_lastShown.start();
}

void PopupPlacer::hide() {
    if (!m_window->isVisible())
        return;
    m_window->hide();
    if (m_clickCatcher)
        m_clickCatcher->hide();
    m_lastHidden.start();
}

void PopupPlacer::toggle() {
    // Clicking the tray icon first takes focus from the popup, which hides
    // it; the click itself then arrives a moment later. Without this guard
    // the popup would flash closed and straight back open.
    if (m_lastHidden.isValid() && m_lastHidden.elapsed() < 350)
        return;
    if (m_window->isVisible())
        hide();
    else
        show();
}

bool PopupPlacer::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_window && event->type() == QEvent::FocusOut)
        dismissIfInactive();
    return QObject::eventFilter(watched, event);
}

void PopupPlacer::dismissIfInactive() {
    // Ignore the focus churn right after opening.
    if (m_lastShown.isValid() && m_lastShown.elapsed() < 300)
        return;
    QTimer::singleShot(0, this, [this] {
        if (m_window && m_window->isVisible() && !m_window->isActive())
            hide();
    });
}
