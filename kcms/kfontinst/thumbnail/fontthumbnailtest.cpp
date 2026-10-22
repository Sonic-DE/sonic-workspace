/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <KIO/ThumbnailCreator>
#include <KPluginFactory>
#include <KPluginMetaData>

#include <QApplication>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>

#include <fontconfig/fontconfig.h>
#include <memory>

namespace {
QProcessEnvironment previewEnvironment()
{
    auto environment = QProcessEnvironment::systemEnvironment();
    // Exercise the freshly built engine, even if the caller supplies an older
    // installation prefix through LD_LIBRARY_PATH.
    environment.insert(QStringLiteral("LD_LIBRARY_PATH"),
                       QStringLiteral(FONT_THUMBNAIL_LIB_DIR) + QLatin1Char(':') + environment.value(QStringLiteral("LD_LIBRARY_PATH")));
    return environment;
}

QString fontFile()
{
    auto *pattern = FcPatternCreate();
    FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8 *>("sans-serif"));
    FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult result;
    auto *match = FcFontMatch(nullptr, pattern, &result);
    FcPatternDestroy(pattern);
    FcChar8 *file = nullptr;
    QString path;
    if (match && FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch) {
        path = QFile::decodeName(reinterpret_cast<const char *>(file));
    }
    if (match) {
        FcPatternDestroy(match);
    }
    return path;
}

bool render(KIO::ThumbnailCreator *creator, const QString &path, const QSize &size)
{
    const auto result = creator->create(KIO::ThumbnailRequest(QUrl::fromLocalFile(path), size, QStringLiteral("font/ttf"), 1.0, 0));
    const auto image = result.image();
    if (!result.isValid() || image.isNull()) {
        qWarning() << "Font preview failed for" << size;
        return false;
    }
    bool ink = false, transparent = false;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const int alpha = image.pixelColor(x, y).alpha();
            ink |= alpha > 128;
            transparent |= alpha == 0;
        }
    }
    return ink && transparent;
}

int runCase(int argc, char **argv, const QString &mode)
{
    const bool noDisplay = mode == QLatin1String("offscreen-no-display");
    qputenv("QT_QPA_PLATFORM", mode.startsWith(QLatin1String("offscreen")) || mode == QLatin1String("repeated") ? "offscreen" : "xcb");
    if (noDisplay) {
        qputenv("DISPLAY", ":65530");
    }
    std::unique_ptr<KIO::ThumbnailCreator> creator;
    {
        QApplication app(argc, argv);
        const auto factory = KPluginFactory::instantiatePlugin<KIO::ThumbnailCreator>(KPluginMetaData(QStringLiteral(FONT_THUMBNAIL_PLUGIN)));
        if (!factory) {
            qWarning() << factory.errorString;
            return 1;
        }
        creator.reset(factory.plugin);
        const QString path = fontFile();
        if (path.isEmpty()) {
            return 1;
        }
        if (noDisplay) {
            const auto result = creator->create(KIO::ThumbnailRequest(QUrl::fromLocalFile(path), QSize(128, 128), QStringLiteral("font/ttf"), 1.0, 0));
            if (result.isValid()) {
                return 1;
            }
        } else {
            const int iterations = mode == QLatin1String("repeated") ? 20 : 1;
            for (int i = 0; i < iterations; ++i) {
                if (!render(creator.get(), path, QSize(128, 128)) || !render(creator.get(), path, QSize(240, 160))) {
                    return 1;
                }
                if (i + 1 < iterations) {
                    creator.reset();
                    const auto next = KPluginFactory::instantiatePlugin<KIO::ThumbnailCreator>(KPluginMetaData(QStringLiteral(FONT_THUMBNAIL_PLUGIN)));
                    if (!next) {
                        qWarning() << next.errorString;
                        return 1;
                    }
                    creator.reset(next.plugin);
                }
            }
        }
        if (mode != QLatin1String("after-application")) {
            creator.reset();
        }
    }
    // Qt has closed its XCB connection. The engine must not retain that Display.
    creator.reset();
    return 0;
}
}

class FontThumbnailTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void previews_data()
    {
        QTest::addColumn<QString>("mode");
        for (const auto *mode : {"offscreen", "offscreen-no-display", "xcb", "after-application", "repeated"}) {
            QTest::newRow(mode) << QString::fromLatin1(mode);
        }
    }

    void previews()
    {
        QFETCH(QString, mode);
        if (mode != QLatin1String("offscreen-no-display") && qgetenv("SONIC_FONT_TEST_ISOLATED") != "1") {
            QSKIP("Rendering requires the private Xvfb CTest wrapper");
        }
        QProcess process;
        process.setProcessEnvironment(previewEnvironment());
        const auto stop = qScopeGuard([&] {
            if (process.state() != QProcess::NotRunning) {
                process.kill();
                process.waitForFinished();
            }
        });
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--case"), mode});
        QVERIFY(process.waitForStarted());
        QVERIFY(process.waitForFinished(20000));
        const auto output = process.readAll();
        QVERIFY2(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, output.constData());
    }

    void concurrentWorkers()
    {
        if (qgetenv("SONIC_FONT_TEST_ISOLATED") != "1") {
            QSKIP("Rendering requires the private Xvfb CTest wrapper");
        }
        QProcess workers[3];
        const auto stop = qScopeGuard([&] {
            for (auto &worker : workers) {
                if (worker.state() != QProcess::NotRunning) {
                    worker.kill();
                    worker.waitForFinished();
                }
            }
        });
        for (auto &worker : workers) {
            worker.setProcessEnvironment(previewEnvironment());
            worker.setProcessChannelMode(QProcess::MergedChannels);
            worker.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--case"), QStringLiteral("offscreen")});
            QVERIFY(worker.waitForStarted());
        }
        for (auto &worker : workers) {
            if (worker.state() != QProcess::NotRunning) {
                QVERIFY(worker.waitForFinished(20000));
            }
            const auto output = worker.readAll();
            QVERIFY2(worker.exitStatus() == QProcess::NormalExit && worker.exitCode() == 0, output.constData());
        }
    }
};

int main(int argc, char **argv)
{
    QTemporaryDir home;
    if (!home.isValid()) {
        return 1;
    }
    qputenv("HOME", home.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", (home.path() + QStringLiteral("/config")).toUtf8());
    qputenv("XDG_CONFIG_DIRS", (home.path() + QStringLiteral("/empty-config")).toUtf8());
    qputenv("XDG_CACHE_HOME", (home.path() + QStringLiteral("/cache")).toUtf8());
    qputenv("QT_QPA_PLATFORMTHEME", "generic");
    qputenv("QT_NO_XDG_DESKTOP_PORTAL", "1");
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/font-thumbnail-test-bus");
    qputenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/nonexistent/font-thumbnail-test-system-bus");
    qputenv("QT_SCALE_FACTOR", "1");
    qunsetenv("QT_SCREEN_SCALE_FACTORS");
    if (argc == 3 && QByteArray(argv[1]) == "--case") {
        return runCase(argc, argv, QString::fromLocal8Bit(argv[2]));
    }
    QCoreApplication app(argc, argv);
    FontThumbnailTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "fontthumbnailtest.moc"
