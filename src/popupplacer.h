#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QRect>

class QQuickWindow;
class QSystemTrayIcon;
class QWindow;

// Shows the popup next to the tray and hides it when it loses focus, the way
// a tray flyout behaves.
//
// Omarchy/Hyprland (Wayland): a layer-shell surface anchored to the corner
// of the screen where the bar's tray is, since Wayland windows cannot place
// themselves. Windows (and X11): a frameless tool window positioned against
// the tray icon, or the taskbar corner when the icon's rect is unknown.
class PopupPlacer : public QObject {
    Q_OBJECT
public:
    PopupPlacer(QQuickWindow *window, QSystemTrayIcon *tray, QObject *parent = nullptr);

    void toggle();
    void show();
    void hide();
    bool isLayerShell() const { return m_layerShell; }

    // Where the Omarchy bar's tray lives: "top-right", "bottom-left", ...
    static QString omarchyTrayCorner(const QString &shellJson);
    // Top-left of a popup of size `popup` docked to the taskbar corner nearest
    // the tray icon. Pure function so it can be tested.
    static QPoint dockedPosition(const QRect &screen, const QRect &available, const QRect &trayIcon,
                                 const QSize &popup, int margin);

private:
    void setupLayerShell();
    void placeFloating();
    bool eventFilter(QObject *watched, QEvent *event) override;
    void dismissIfInactive();

    QPointer<QQuickWindow> m_window;
    // Wayland only: a transparent full-screen layer under the popup that
    // closes it on any click outside, since a layer surface that never took
    // keyboard focus gets no focus-out.
    QWindow *m_clickCatcher = nullptr;
    QSystemTrayIcon *m_tray;
    bool m_layerShell = false;
    QElapsedTimer m_lastHidden;
    QElapsedTimer m_lastShown;
};
