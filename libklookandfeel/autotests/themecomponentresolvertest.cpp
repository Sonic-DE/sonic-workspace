/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "themecomponentresolver.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTest>

class ThemeComponentResolverTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        const QString data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        QDir().mkpath(data + QStringLiteral("/color-schemes"));
        QDir().mkpath(data + QStringLiteral("/icons/thirdparty/cursors"));
        QDir().mkpath(data + QStringLiteral("/plasma/desktoptheme/expose"));
        QDir().mkpath(data + QStringLiteral("/plasma/look-and-feel/org.magpie.expose.desktop"));
        QFile color(data + QStringLiteral("/color-schemes/ThirdParty.colors"));
        QVERIFY(color.open(QIODevice::WriteOnly));
        QFile plasma(data + QStringLiteral("/plasma/desktoptheme/expose/metadata.json"));
        QVERIFY(plasma.open(QIODevice::WriteOnly));
        QFile lookAndFeel(data + QStringLiteral("/plasma/look-and-feel/org.magpie.expose.desktop/metadata.json"));
        QVERIFY(lookAndFeel.open(QIODevice::WriteOnly));
        QDir().mkpath(data + QStringLiteral("/icons/inherited-cursor"));
        QFile inherited(data + QStringLiteral("/icons/inherited-cursor/index.theme"));
        QVERIFY(inherited.open(QIODevice::WriteOnly));
        inherited.write("[Icon Theme]\nInherits=thirdparty\n");
        QDir().mkpath(data + QStringLiteral("/icons/cursor-cycle-a"));
        QDir().mkpath(data + QStringLiteral("/icons/cursor-cycle-b"));
        QFile cycleA(data + QStringLiteral("/icons/cursor-cycle-a/index.theme"));
        QVERIFY(cycleA.open(QIODevice::WriteOnly));
        cycleA.write("[Icon Theme]\nInherits=cursor-cycle-b\n");
        QFile cycleB(data + QStringLiteral("/icons/cursor-cycle-b/index.theme"));
        QVERIFY(cycleB.open(QIODevice::WriteOnly));
        cycleB.write("[Icon Theme]\nInherits=cursor-cycle-a\n");
    }

    void preservesThirdPartyPackages()
    {
        QVERIFY(ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::ColorScheme, QStringLiteral("ThirdParty")));
        QVERIFY(ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::CursorTheme, QStringLiteral("thirdparty")));
        QVERIFY(ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::PlasmaTheme, QStringLiteral("expose")));
        QVERIFY(ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::LookAndFeel, QStringLiteral("org.magpie.expose.desktop")));
    }

    void detectsMissingPackages()
    {
        QVERIFY(!ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::ColorScheme, QStringLiteral("missing")));
        QVERIFY(!ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::CursorTheme, QStringLiteral("missing")));
        QVERIFY(!ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::PlasmaTheme, QStringLiteral("missing")));
        QVERIFY(!ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::LookAndFeel, QStringLiteral("missing")));
    }

    void followsCursorInheritanceSafely()
    {
        QVERIFY(ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::CursorTheme, QStringLiteral("inherited-cursor")));
        QVERIFY(!ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::CursorTheme, QStringLiteral("cursor-cycle-a")));
    }

    void acceptsDelegatingThirdPartyDirectories()
    {
        const QString data = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        QDir().mkpath(data + QStringLiteral("/plasma/desktoptheme/delegated-no-metadata"));
        QDir().mkpath(data + QStringLiteral("/plasma/look-and-feel/vendor-no-metadata"));
        QVERIFY(ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::PlasmaTheme, QStringLiteral("delegated-no-metadata")));
        QVERIFY(ThemeComponentResolver::isAvailable(ThemeComponentResolver::Type::LookAndFeel, QStringLiteral("vendor-no-metadata")));
    }

    void selectsVariantFallbacks()
    {
        QCOMPARE(ThemeComponentResolver::fallback(ThemeComponentResolver::Type::ColorScheme, false), QStringLiteral("SilverLight"));
        QCOMPARE(ThemeComponentResolver::fallback(ThemeComponentResolver::Type::ColorScheme, true), QStringLiteral("SilverDark"));
        QCOMPARE(ThemeComponentResolver::fallback(ThemeComponentResolver::Type::PlasmaTheme, true), QStringLiteral("silver-dark"));
        QCOMPARE(ThemeComponentResolver::fallback(ThemeComponentResolver::Type::Decoration, false), QStringLiteral("org.kde.silver"));
    }
};

QTEST_MAIN(ThemeComponentResolverTest)
#include "themecomponentresolvertest.moc"
