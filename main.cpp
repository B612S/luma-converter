#include <QApplication>
#include <cstdio>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QIcon>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>
#include <QTranslator>
#include <QUrl>

#include "core/ConfigManager.hpp"
#include "core/ConversionQueue.hpp"
#include "core/Logger.hpp"

#ifndef LUMA_CONVERTER_VERSION
#define LUMA_CONVERTER_VERSION "dev"
#endif

namespace {

// The interface strings are translation keys, so a translator is always needed:
// unsupported system languages fall back to English instead of raw keys.
QString resolveLanguage(const QString &requested) {
    if (requested == QLatin1String("en") || requested == QLatin1String("ru"))
        return requested;
    if (requested != QLatin1String("auto"))
        return QStringLiteral("en");
    const QString system = QLocale::system().name().left(2);
    return system == QLatin1String("ru") ? QStringLiteral("ru") : QStringLiteral("en");
}

// Installed translations live next to the binary's data dir; the resource copy
// always matches this build and is tried first.
void applyLanguage(QTranslator *translator, const QString &requested) {
    QCoreApplication::removeTranslator(translator);
    const QString language = resolveLanguage(requested);
    const bool loaded =
        translator->load(QStringLiteral(":/i18n/") + language + QStringLiteral(".qm"))
        || translator->load(QStringLiteral("/usr/share/luma-converter/translations/")
                            + language + QStringLiteral(".qm"));
    if (loaded)
        QCoreApplication::installTranslator(translator);
}

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("luma-converter"));
    app.setApplicationDisplayName(QStringLiteral("Luma Converter"));
    app.setOrganizationName(QStringLiteral("B612S"));
    app.setApplicationVersion(QString::fromLatin1(LUMA_CONVERTER_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("luma-converter"));
    app.setWindowIcon(QIcon(QStringLiteral(":/assets/luma-converter.png")));

    // Match Luma: use the KDE controls style even when the app is started
    // outside the Plasma launcher, which normally sets this environment
    // variable. Without it Qt falls back to Basic and the settings lose the
    // Breeze/Kirigami look.
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    // Breeze drives the Qt Widgets side (file dialogs) and qqc2-desktop-style,
    // which supplies the Kirigami controls look. Creating the style plugin is
    // skipped on headless platforms: it waits for a desktop that does not exist
    // there and would stall the process.
    const QString platform = QGuiApplication::platformName();
    if (platform != QLatin1String("offscreen") && platform != QLatin1String("minimal")) {
        const QStringList styleKeys = QStyleFactory::keys();
        for (const QString &key : styleKeys) {
            if (key.compare(QStringLiteral("Breeze"), Qt::CaseInsensitive) != 0)
                continue;
            if (QStyle *style = QStyleFactory::create(key))
                QApplication::setStyle(style);
            break;
        }
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Luma Converter: converts audio files with FFmpeg."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption selfTestOption(QStringLiteral("self-test"),
                                      QStringLiteral("Load the QML interface, then exit."));
    QCommandLineOption screenshotOption(QStringLiteral("screenshot"),
                                        QStringLiteral("Save a window screenshot and exit."),
                                        QStringLiteral("file"));
    parser.addOption(selfTestOption);
    parser.addOption(screenshotOption);
    parser.addPositionalArgument(QStringLiteral("files"),
                                 QStringLiteral("Audio files to add to the queue."),
                                 QStringLiteral("[files...]"));
    parser.process(app);

    ConfigManager config;
    Logger::instance().setMode(config.logLevel());

    auto *translator = new QTranslator(&app);
    applyLanguage(translator, config.language());

    ConversionQueue queue(&config);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("Config"), &config);
    engine.rootContext()->setContextProperty(QStringLiteral("Queue"), &queue);
    engine.rootContext()->setContextProperty(QStringLiteral("AppVersion"),
                                             QString::fromLatin1(LUMA_CONVERTER_VERSION));
    engine.rootContext()->setContextProperty(QStringLiteral("QtVersion"),
                                             QString::fromLatin1(qVersion()));

    const QUrl url(QStringLiteral("qrc:/qt/qml/LumaConverter/qml/Main.qml"));
    bool created = true;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                     [&created, &url](QObject *object, const QUrl &objectUrl) {
                         if (objectUrl == url && !object)
                             created = false;
                     });
    engine.load(url);
    if (!created || engine.rootObjects().isEmpty())
        return 1;

    // Switching the language re-installs the translator and lets the engine
    // re-evaluate every qsTr binding, as Luma does.
    QObject::connect(&config, &ConfigManager::languageChanged, &app,
                     [&config, translator, &engine]() {
                         applyLanguage(translator, config.language());
                         engine.retranslate();
                     });

    const QStringList initialFiles = parser.positionalArguments();
    if (!initialFiles.isEmpty()) {
        queue.addFiles(initialFiles);
        if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst()))
            window->requestActivate();
    }

    if (parser.isSet(selfTestOption) || parser.isSet(screenshotOption)) {
        QTimer::singleShot(1200, &app, [&] {
            if (parser.isSet(screenshotOption)) {
                auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
                if (window) {
                    const QImage image = window->grabWindow();
                    if (image.isNull()) {
                        qWarning("screenshot: grabWindow() returned a null image");
                    } else if (!image.save(parser.value(screenshotOption))) {
                        qWarning("screenshot: failed to save %s",
                                 qPrintable(parser.value(screenshotOption)));
                    } else {
                        qInfo("screenshot: saved %s (%dx%d)",
                              qPrintable(parser.value(screenshotOption)),
                              image.width(), image.height());
                    }
                } else {
                    qWarning("screenshot: no window to grab");
                }
            }
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
