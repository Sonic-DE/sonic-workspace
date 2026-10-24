/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickView>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>

class WallpaperFaderTest : public QObject {
    Q_OBJECT

    static double edgeEnergy(const QImage &image, const QRect &area)
    {
        double sum = 0;
        qsizetype count = 0;
        for (int y = area.top(); y <= area.bottom(); ++y) {
            for (int x = area.left(); x <= area.right(); ++x) {
                const int pixel = qGray(image.pixel(x, y));
                if (x < area.right()) {
                    sum += std::abs(pixel - qGray(image.pixel(x + 1, y)));
                    ++count;
                }
                if (y < area.bottom()) {
                    sum += std::abs(pixel - qGray(image.pixel(x, y + 1)));
                    ++count;
                }
            }
        }
        return count ? sum / count : 0;
    }

private Q_SLOTS:
    void windowColorTint_data()
    {
        QTest::addColumn<QColor>("background");
        QTest::addColumn<QColor>("wallpaper");
        const QColor neutral(128, 128, 128);
        QTest::newRow("dark") << QColor("#181b28") << neutral;
        QTest::newRow("light") << QColor("#eff0f1") << neutral;
        QTest::newRow("saturated-blue") << QColor("#2020c0") << neutral;
        QTest::newRow("warm") << QColor("#c06020") << neutral;
        QTest::newRow("translucent") << QColor(32, 64, 192, 128) << neutral;
        QTest::newRow("colored-wallpaper") << QColor("#181b28") << QColor(64, 160, 208);
    }

    void windowColorTint()
    {
        if (qgetenv("SONIC_WALLPAPER_TEST_ISOLATED") != "1") {
            QSKIP("Rendered tint test requires the private Xvfb test wrapper");
        }
        QFETCH(QColor, background);
        QFETCH(QColor, wallpaper);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QImage image(320, 240, QImage::Format_RGB32);
        image.fill(wallpaper);
        const auto path = fixture.path() + QStringLiteral("/wallpaper.png");
        QVERIFY(image.save(path));
        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.resize(image.size());
        view.setInitialProperties({{QStringLiteral("testImage"), QUrl::fromLocalFile(path)}, {QStringLiteral("windowBackground"), background}});
        view.setSource(QUrl::fromLocalFile(QStringLiteral(WALLPAPER_FADER_TEST_QML)));
        QVERIFY2(view.status() == QQuickView::Ready, qPrintable(view.errors().isEmpty() ? QString() : view.errors().first().toString()));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto *fader = view.rootObject()->property("fader").value<QQuickItem *>();
        QVERIFY(fader);
        QTRY_COMPARE(fader->property("tintColor").value<QColor>(), background);
        const auto matches = [&view, wallpaper](const QColor &tint, double factor) {
            const auto frame = view.grabWindow();
            if (frame.isNull()) {
                return false;
            }
            const auto actual = frame.pixelColor(frame.width() / 2, frame.height() / 2);
            const double amount = 0.35 * factor * tint.alphaF();
            const auto expected = [amount](int source, int tint) {
                return qRound(source * (1 - amount) + tint * amount);
            };
            return std::abs(actual.red() - expected(wallpaper.red(), tint.red())) <= 2
                && std::abs(actual.green() - expected(wallpaper.green(), tint.green())) <= 2
                && std::abs(actual.blue() - expected(wallpaper.blue(), tint.blue())) <= 2 && actual.alpha() == 255;
        };
        QTRY_VERIFY(matches(background, 0));
        QVERIFY(view.rootObject()->setProperty("blurred", true));
        QTRY_COMPARE_WITH_TIMEOUT(fader->property("factor").toDouble(), 1.0, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(matches(background, 1), 5000);
        QVERIFY(fader->setProperty("factor", 0.5));
        QTRY_COMPARE_WITH_TIMEOUT(fader->property("factor").toDouble(), 0.5, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(matches(background, 0.5), 5000);
        const QColor alternate("#883355");
        QVERIFY(view.rootObject()->setProperty("windowBackground", alternate));
        QTRY_VERIFY_WITH_TIMEOUT(matches(alternate, 0.5), 5000);
        QVERIFY(view.rootObject()->setProperty("blurred", false));
        QTRY_COMPARE_WITH_TIMEOUT(fader->property("factor").toDouble(), 0.0, 5000);
        QTRY_VERIFY(matches(alternate, 0));
    }

    void blurCoversEveryEdge_data()
    {
        QTest::addColumn<QSize>("size");
        QTest::addColumn<bool>("layered");
        QTest::newRow("layered-640x400") << QSize(640, 400) << true;
        QTest::newRow("layered-non-power-of-two") << QSize(901, 507) << true;
        QTest::newRow("texture-provider-640x400") << QSize(640, 400) << false;
    }

    void blurCoversEveryEdge()
    {
        if (qgetenv("SONIC_WALLPAPER_TEST_ISOLATED") != "1") {
            QSKIP("Rendered blur test requires the private Xvfb test wrapper");
        }
        QFETCH(QSize, size);
        QFETCH(bool, layered);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QImage checkerboard(size, QImage::Format_RGB32);
        for (int y = 0; y < size.height(); ++y) {
            for (int x = 0; x < size.width(); ++x) {
                checkerboard.setPixel(x, y, ((x / 4 + y / 4) % 2) ? qRgb(240, 240, 240) : qRgb(16, 16, 16));
            }
        }
        const QString path = fixture.path() + QStringLiteral("/checkerboard.png");
        QVERIFY(checkerboard.save(path));
        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.resize(size);
        view.setInitialProperties({{QStringLiteral("testImage"), QUrl::fromLocalFile(path)}, {QStringLiteral("sourceLayered"), layered}});
        view.setSource(QUrl::fromLocalFile(QStringLiteral(WALLPAPER_FADER_TEST_QML)));
        QVERIFY2(view.status() == QQuickView::Ready, qPrintable(view.errors().isEmpty() ? QString() : view.errors().first().toString()));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto *fader = view.rootObject()->property("fader").value<QQuickItem *>();
        QVERIFY(fader);
        QTRY_COMPARE(fader->property("factor").toDouble(), 0.0);
        QImage clear;
        QTRY_VERIFY(!(clear = view.grabWindow()).isNull());
        QCOMPARE(clear.size(), size);
        const QList<QRect> regions = {
            QRect(0, 0, size.width(), 24),
            QRect(0, size.height() - 24, size.width(), 24),
            QRect(0, 0, 24, size.height()),
            QRect(size.width() - 24, 0, 24, size.height()),
            QRect(size.width() / 2 - 40, size.height() / 2 - 40, 80, 80),
        };
        for (const auto &region : regions) {
            QVERIFY2(edgeEnergy(clear, region) > 30, "Clear reference must contain sharp checkerboard detail");
        }
        QVERIFY(view.rootObject()->setProperty("blurred", true));
        QTRY_COMPARE_WITH_TIMEOUT(fader->property("factor").toDouble(), 1.0, 5000);
        for (qsizetype i = 0; i < regions.size(); ++i) {
            const QRect region = regions[i];
            const double maximum = edgeEnergy(clear, region) * 0.25;
            QTRY_VERIFY2_WITH_TIMEOUT(edgeEnergy(view.grabWindow(), region) < maximum,
                                      qPrintable(QStringLiteral("Unblurred detail remains in region %1").arg(i)),
                                      5000);
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
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/wallpaper-fader-test-bus");
    qputenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/nonexistent/wallpaper-fader-test-system-bus");
    qputenv("QT_SCALE_FACTOR", "1");
    qunsetenv("QT_SCREEN_SCALE_FACTORS");
    qunsetenv("QT_QUICK_BACKEND");
    QGuiApplication app(argc, argv);
    WallpaperFaderTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "wallpaperfadertest.moc"
