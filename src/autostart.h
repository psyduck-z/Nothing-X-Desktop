#pragma once

#include <QString>

// "Launch at login": an XDG autostart entry on Linux (honoured by Omarchy's
// uwsm session), the per-user Run key on Windows.
namespace Autostart {
bool isEnabled();
void setEnabled(bool on);
QString linuxEntryPath();
}
