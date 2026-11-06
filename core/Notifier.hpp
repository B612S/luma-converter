#pragma once
#include <QString>

// Desktop notifications through org.freedesktop.Notifications.
// Silently does nothing when no session bus is available (e.g. headless runs).
class Notifier {
public:
    static bool available();
    static void send(const QString &title, const QString &body);
};
