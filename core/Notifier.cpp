#include "Notifier.hpp"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QVariantMap>

bool Notifier::available() {
    return QDBusConnection::sessionBus().isConnected();
}

void Notifier::send(const QString &title, const QString &body) {
    if (!available())
        return;

    QVariantMap hints;
    hints.insert(QStringLiteral("desktop-entry"), QStringLiteral("luma-converter"));
    hints.insert(QStringLiteral("urgency"), QStringLiteral("normal"));

    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("Notify"));

    message << QStringLiteral("Luma Converter")   // app name
            << 0u                                 // replaces id
            << QStringLiteral("luma-converter")   // app icon
            << title
            << body
            << QStringList()                      // actions
            << hints
            << 6000;                              // timeout, ms

    QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
}
