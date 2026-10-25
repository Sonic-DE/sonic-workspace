/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
    SPDX-FileCopyrightText: 2026 SonicDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "debug.h"
#include "legacyshellmigration.h"
#include "shellcorona.h"
#include "softwarerendernotifier.h"
#include "themecomponentrepaircoordinator.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QDir>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProcess>
#include <QQmlDebuggingEnabler>
#include <QQuickWindow>
#include <QSurfaceFormat>

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <KSignalHandler>

#include <csignal>

#if __has_include(<malloc.h>)
#include <malloc.h>
#include <unistd.h>
#endif

#if __has_include(<malloc.h>) && defined(__GLIBC__)
static void setupMalloc()
{
    // The default threshold is 128 * 1024, which can result in a large memory usage due to
    // fragmentation especially if we use the raster graphicssystem. On the other side if the
    // threshold is too low, free() starts to permanently ask the kernel about shrinking the heap.
    //
    // Setting M_TRIM_THRESHOLD also disables dynamic adjustment of M_MMAP_THRESHOLD, which is
    // important with wallpapers. The average amount of memory necessary to store a 4K wallpaper
    // is just under 32MB, the upper limit for M_MMAP_THRESHOLD. By disabling dynamic adjustment
    // of M_MMAP_THRESHOLD, we ensure that memory for wallpapers is allocated with direct mmaps
    // and released to the system individually without causing further fragmentation.
    const int pagesize = sysconf(_SC_PAGESIZE);
    mallopt(M_TRIM_THRESHOLD, 5 * pagesize);
}
#endif

int main(int argc, char *argv[])
{
    // Import before QApplication/platform plugins or any shell constructor can
    // open the new configuration. Help/version probes must not migrate a profile.
    QString migrationShell;
    bool informationOnly = false;
    for (int i = 1; i < argc; ++i) {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        if (argument == QStringLiteral("--")) {
            break;
        }
        if (argument == QStringLiteral("--version") || argument == QStringLiteral("-v") || argument == QStringLiteral("--help")
            || argument == QStringLiteral("-h") || argument == QStringLiteral("--help-all") || argument == QStringLiteral("--author")
            || argument == QStringLiteral("--license")) {
            informationOnly = true;
        } else if (argument == QStringLiteral("--shell-plugin") || argument == QStringLiteral("-p")) {
            if (++i == argc) {
                qCritical("Missing shell package argument");
                return 2;
            }
            migrationShell = QString::fromLocal8Bit(argv[i]);
        } else if (argument.startsWith(QStringLiteral("--shell-plugin="))) {
            migrationShell = argument.mid(QStringLiteral("--shell-plugin=").size());
        } else if (argument.startsWith(QStringLiteral("-p")) && !argument.startsWith(QStringLiteral("-platform"))
                   && !argument.startsWith(QStringLiteral("-plugin"))) {
            migrationShell = argument.mid(2);
        }
    }
    if (!informationOnly) {
        QString error;
        if (!SonicShellMigration::importSettings(SonicShellMigration::userPaths(), migrationShell, &error)) {
            qCritical().noquote() << "Sonic Shell settings import failed:" << error;
            return 1;
        }
    }

#if __has_include(<malloc.h>) && defined(__GLIBC__)
    setupMalloc();
#endif

#if QT_CONFIG(qml_debug)
    if (qEnvironmentVariableIsSet("PLASMA_ENABLE_QML_DEBUG")) {
        QQmlDebuggingEnabler::enableDebugging(true);
    }
#endif

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QQuickWindow::setDefaultAlphaBuffer(true);

    // Qt's automatic pipeline cache stores only one window's RHI data at a time.
    // Disable it until per-window caching is implemented; gains are minimal for the shell.
    // This applies to X11/OpenGL as well as other RHI backends.
    // https://doc.qt.io/qt-6/qquickgraphicsconfiguration.html#the-automatic-pipeline-cache
    QCoreApplication::setAttribute(Qt::AA_DisableShaderDiskCache);

    QApplication app(argc, argv);

    // Quit on SIGTERM to properly save state. See systemd.kill(5).
    // https://bugs.kde.org/show_bug.cgi?id=470604
    KSignalHandler::self()->watchSignal(SIGTERM);
    QObject::connect(KSignalHandler::self(), &KSignalHandler::signalReceived, &app, [&app](int signal) {
        if (signal == SIGTERM) {
            app.quit();
        }
    });

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("sonicshell"));

    // The executable's path is added to the library/plugin paths.
    // This does not make much sense for plasmashell.
    app.removeLibraryPath(QCoreApplication::applicationDirPath());

    KAboutData aboutData(QStringLiteral("sonicshell"), i18n("Sonic Shell"), QStringLiteral(PROJECT_VERSION), i18n("Sonic Shell"), KAboutLicense::GPL);
    aboutData.addAuthor(i18n("SonicDE contributors"));
    aboutData.setOrganizationDomain(QByteArrayLiteral("sonicde.org"));
    aboutData.setDesktopFileName(QStringLiteral("org.sonicde.sonicshell"));

    KAboutData::setApplicationData(aboutData);
    KCrash::initialize();

    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser cliOptions;

    QCommandLineOption dbgOption(QStringList() << QStringLiteral("d") << QStringLiteral("qmljsdebugger"), i18n("Enable QML Javascript debugger"));

    QCommandLineOption noRespawnOption(QStringList() << QStringLiteral("n") << QStringLiteral("no-respawn"),
                                       i18n("Do not restart Sonic Shell automatically after a crash"));

    QCommandLineOption shellPluginOption(QStringList() << QStringLiteral("p") << QStringLiteral("shell-plugin"),
                                         i18n("Force loading the given shell plugin"),
                                         QStringLiteral("plugin"),
                                         ShellCorona::defaultShell());

    QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));

    cliOptions.addOption(dbgOption);
    cliOptions.addOption(noRespawnOption);
    cliOptions.addOption(shellPluginOption);
    cliOptions.addOption(replaceOption);

    aboutData.setupCommandLine(&cliOptions);
    cliOptions.process(app);
    aboutData.processCommandLine(&cliOptions);

    // don't let the first KJob terminate us
    QCoreApplication::setQuitLockEnabled(false);

    // A system-wide default may select a different shell from the user's
    // local configuration. Import that layout before constructing Corona.
    QString layoutImportError;
    if (!SonicShellMigration::importSettings(SonicShellMigration::userPaths(), cliOptions.value(shellPluginOption), &layoutImportError)) {
        qCritical().noquote() << "Sonic Shell layout import failed:" << layoutImportError;
        return 1;
    }
    ShellCorona corona;
    corona.setShell(cliOptions.value(shellPluginOption));
    if (!corona.kPackage().isValid()) {
        qCritical() << "starting invalid corona" << corona.shell();
        return 1;
    }

    if (!cliOptions.isSet(noRespawnOption)) {
        KCrash::setFlags(KCrash::AutoRestart);
    }

    // Tells libnotificationmanager that we're the only true application that may own notification and job progress services
    qApp->setProperty("_plasma_dbus_master", true);

    QObject::connect(&corona, &ShellCorona::glInitializationFailed, &app, [&app]() {
        // scene graphs errors come from a thread
        // even though we process them in the main thread, app.exit could still process these events
        static bool s_multipleInvokations = false;
        if (s_multipleInvokations) {
            return;
        }
        s_multipleInvokations = true;

        qCritical("Open GL context could not be created");
        auto configGroup = KSharedConfig::openConfig()->group(QStringLiteral("QtQuickRendererSettings"));
        if (configGroup.readEntry("SceneGraphBackend") != QLatin1String("software")) {
            configGroup.writeEntry("SceneGraphBackend", "software", KConfigBase::Global | KConfigBase::Persistent);
            configGroup.sync();
            QProcess::startDetached(QStringLiteral("sonicshell"), app.arguments());
        } else {
            QCoreApplication::setAttribute(Qt::AA_ForceRasterWidgets);
            QMessageBox::critical(nullptr,
                                  i18n("Sonic Shell Failed To Start"),
                                  i18n("Sonic Shell is unable to start as it could not correctly use OpenGL 2 or software fallback\nPlease check that your "
                                       "graphic drivers are set up correctly."));
        }
        app.exit(-1);
    });
    const bool replace = cliOptions.isSet(replaceOption);

    KDBusService service(KDBusService::Unique | KDBusService::StartupOption(replace ? KDBusService::Replace : 0));

    corona.init();
    auto themeRepairCoordinator = new ThemeComponentRepairCoordinator(&app);
    themeRepairCoordinator->start();
    SoftwareRendererNotifier::notifyIfRelevant();

    return app.exec();
}
