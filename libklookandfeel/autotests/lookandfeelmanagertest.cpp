/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "klookandfeelmanager.h"
// Qt
#include <KJob>
#include <KPackage/Package>
#include <KPackage/PackageJob>
#include <KPackage/PackageLoader>
#include <KSharedConfig>
#include <KSycoca>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class LookAndFeelManagerTest : public QObject
{
    /**
     *  Following tests ensure lookandfeel configuration are saved to kdedefaults,
     *  and configuration (kdeglobals, kwinrc...) from XDG_CONFIG_HOME return default value.
     *
     *  Because test don't have a modified XDG_CONFIG_DIRS with kdedefault dir added,
     *  we can easily check XDG_CONFIG_HOME values have default value and still using cascade/global behavior
     */
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testWidgetStyle();
    void testColors();
    void testIcons();
    void testPlasmaTheme();
    void testCursorTheme();
    void testSplashScreen();
    void testWindowSwitcher();
    void testSave();
    void testThemeTransparency();

private:
    QDir m_configDir;
    QDir m_dataDir;
    KLookAndFeelManager *m_lookAndFeel;
};

void LookAndFeelManagerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qunsetenv("XDG_CONFIG_DIRS");

    m_configDir = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    m_configDir.removeRecursively();

    m_dataDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    m_dataDir.removeRecursively();

    QVERIFY(m_configDir.mkpath(u"."_s));

    // we need an existing colorscheme file, even if empty
    QVERIFY(m_dataDir.mkpath(QStringLiteral("color-schemes")));
    QFile f(m_dataDir.path() + QStringLiteral("/color-schemes/TestValue.colors"));
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file:" << f.errorString();
    }
    f.close();

    const QString packagePath = QFINDTESTDATA("lookandfeel");
    const QString packageRoot = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/plasma/look-and-feel/";
    auto installJob = KPackage::PackageJob::install(QStringLiteral("Plasma/LookAndFeel"), packagePath, packageRoot);
    connect(installJob, &KJob::result, this, [installJob]() {
        QCOMPARE(installJob->errorText(), QString());
        QCOMPARE(installJob->error(), KJob::NoError);
        QVERIFY(installJob->package().isValid());
    });
    QSignalSpy spy(installJob, &KPackage::PackageJob::finished);
    spy.wait();

    KConfig config(u"kdeglobals"_s);
    KConfigGroup cg(&config, u"KDE"_s);
    cg.writeEntry("LookAndFeelPackage", "org.kde.test");
    cg.sync();
    m_lookAndFeel = new KLookAndFeelManager(this);
}

void LookAndFeelManagerTest::cleanupTestCase()
{
    m_configDir.removeRecursively();
    m_dataDir.removeRecursively();
}

void LookAndFeelManagerTest::testWidgetStyle()
{
    // We have to use an actual theme name here because setWidgetStyle checks
    // if the theme can actually be used before it changes the config
    m_lookAndFeel->setWidgetStyle(QStringLiteral("Fusion"));

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, u"KDE"_s);
    QCOMPARE(cg.readEntry("widgetStyle", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"KDE"_s);
    QCOMPARE(cgd.readEntry("widgetStyle", QString()), QStringLiteral("Fusion"));

    // Test that a fake style is not actually written to config
    m_lookAndFeel->setWidgetStyle(QStringLiteral("customStyle"));
    QCOMPARE(cgd.readEntry("widgetStyle", QString()), QStringLiteral("Fusion"));
}

void LookAndFeelManagerTest::testColors()
{
    // TODO: test colorFile as well
    m_lookAndFeel->setColors(QStringLiteral("customTestValue"), QString());

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, u"General"_s);
    QCOMPARE(cg.readEntry("ColorScheme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"General"_s);
    QCOMPARE(cgd.readEntry("ColorScheme", QString()), QStringLiteral("customTestValue"));
}

void LookAndFeelManagerTest::testIcons()
{
    m_lookAndFeel->setIcons(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, u"Icons"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"Icons"_s);
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
}

void LookAndFeelManagerTest::testPlasmaTheme()
{
    m_lookAndFeel->setPlasmaTheme(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("plasmarc"));
    KConfigGroup cg(&config, u"Theme"_s);
    QCOMPARE(cg.readEntry("name", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/plasmarc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"Theme"_s);
    QCOMPARE(cgd.readEntry("name", QString()), QStringLiteral("customTestValue"));
}

void LookAndFeelManagerTest::testCursorTheme()
{
    m_lookAndFeel->setCursorTheme(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kcminputrc"));
    KConfigGroup cg(&config, u"Mouse"_s);
    QCOMPARE(cg.readEntry("cursorTheme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/kcminputrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"Mouse"_s);
    QCOMPARE(cgd.readEntry("cursorTheme", QString()), QStringLiteral("customTestValue"));
}

void LookAndFeelManagerTest::testSplashScreen()
{
    m_lookAndFeel->setSplashScreen(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("ksplashrc"));
    KConfigGroup cg(&config, u"KSplash"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cg.readEntry("Engine", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/ksplashrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"KSplash"_s);
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
    QCOMPARE(cgd.readEntry("Engine", QString()), QStringLiteral("KSplashQML"));
}

void LookAndFeelManagerTest::testWindowSwitcher()
{
    m_lookAndFeel->setWindowSwitcher(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kwinrc"));
    KConfigGroup cg(&config, u"TabBox"_s);
    QCOMPARE(cg.readEntry("LayoutName", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/kwinrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"TabBox"_s);
    QCOMPARE(cgd.readEntry("LayoutName", QString()), QStringLiteral("customTestValue"));
}

void LookAndFeelManagerTest::testSave()
{
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), QStringLiteral("org.kde.test"));
    m_lookAndFeel->save(package, KLookAndFeelManager::Empty);

    // On real setup we read entries from kdedefaults directory (XDG_CONFIG_DIRS is modified but not in test scenario)

    KConfig config(QStringLiteral("kdeglobals"));
    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cg(&config, u"KDE"_s);
    KConfigGroup cgd(&configDefault, u"KDE"_s);
    // See comment in testWidgetStyle
    QCOMPARE(cg.readEntry("widgetStyle", QString()), QString());
    QCOMPARE(cgd.readEntry("widgetStyle", QString()), QStringLiteral("Fusion"));

    cg = KConfigGroup(&config, u"General"_s);
    cgd = KConfigGroup(&configDefault, u"General"_s);
    // save() capitalizes the ColorScheme
    QCOMPARE(cg.readEntry("ColorScheme", QString()), QString());
    QCOMPARE(cgd.readEntry("ColorScheme", QString()), QStringLiteral("customTestValue"));

    cg = KConfigGroup(&config, u"Icons"_s);
    cgd = KConfigGroup(&configDefault, u"Icons"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));

    KConfig plasmaConfig(QStringLiteral("plasmarc"));
    KConfig plasmaConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/plasmarc", KConfig::SimpleConfig);
    cg = KConfigGroup(&plasmaConfig, u"Theme"_s);
    cgd = KConfigGroup(&plasmaConfigDefault, u"Theme"_s);
    QCOMPARE(cg.readEntry("name", QString()), QString());
    QCOMPARE(cgd.readEntry("name", QString()), QStringLiteral("customTestValue"));

    KConfig inputConfig(QStringLiteral("kcminputrc"));
    KConfig inputConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/kcminputrc", KConfig::SimpleConfig);
    cg = KConfigGroup(&inputConfig, u"Mouse"_s);
    cgd = KConfigGroup(&inputConfigDefault, u"Mouse"_s);
    QCOMPARE(cg.readEntry("cursorTheme", QString()), QString());
    QCOMPARE(cgd.readEntry("cursorTheme", QString()), QStringLiteral("customTestValue"));

    KConfig splashConfig(QStringLiteral("ksplashrc"));
    KConfig splashConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/ksplashrc", KConfig::SimpleConfig);
    cg = KConfigGroup(&splashConfig, u"KSplash"_s);
    cgd = KConfigGroup(&splashConfigDefault, u"KSplash"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cg.readEntry("Engine", QString()), QString());
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
    QCOMPARE(cgd.readEntry("Engine", QString()), QStringLiteral("KSplashQML"));

    KConfig kwinConfig(QStringLiteral("kwinrc"));
    KConfig kwinConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/kwinrc", KConfig::SimpleConfig);
    cg = KConfigGroup(&kwinConfig, u"TabBox"_s);
    cgd = KConfigGroup(&kwinConfigDefault, u"TabBox"_s);
    QCOMPARE(cg.readEntry("LayoutName", QString()), QString());
    QCOMPARE(cgd.readEntry("LayoutName", QString()), QStringLiteral("customTestValue"));
}

static KPackage::Package writeTestTheme(const QTemporaryDir &dir, const QByteArray &defaults, const QByteArray &layoutDefaults = {})
{
    QDir(dir.path()).mkpath(u"contents/layouts"_s);
    if (!layoutDefaults.isEmpty()) {
        QFile layout(dir.filePath(u"contents/layouts/defaults"_s));
        layout.open(QIODevice::WriteOnly);
        layout.write(layoutDefaults);
        layout.close();
    }
    QFile metadata(dir.filePath(u"metadata.json"_s));
    metadata.open(QIODevice::WriteOnly);
    metadata.write(R"({"KPackageStructure": "Plasma/LookAndFeel", "KPlugin": {"Id": "org.kde.test.transparency", "Name": "Test"}})");
    metadata.close();
    QFile file(dir.filePath(u"contents/defaults"_s));
    file.open(QIODevice::WriteOnly);
    file.write(defaults);
    file.close();

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(u"Plasma/LookAndFeel"_s);
    package.setPath(dir.path());
    return package;
}

void LookAndFeelManagerTest::testThemeTransparency()
{
    const QByteArray parts =
        "[kdeglobals][KDE]\nwidgetStyle=Fusion\n"
        "[plasmarc][Theme]\nname=testValue\n"
        "[kwinrc][org.kde.kdecoration2]\nlibrary=org.kde.test\n";
    const QByteArray buttons = "[kwinrc][org.kde.kdecoration2]\nButtonsOnLeft=MN\n";
    const KLookAndFeelManager::Contents mask =
        KLookAndFeelManager::WidgetStyle | KLookAndFeelManager::PlasmaTheme | KLookAndFeelManager::WindowDecoration | KLookAndFeelManager::TitlebarLayout;
    const QString defaultsDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/kdedefaults/"_s;

    KSharedConfigPtr user = KSharedConfig::openConfig(u"kdeglobals"_s);
    user->group(u"ToolsArea"_s).writeEntry("ActiveOpacity", 55);
    user->sync();

    QTemporaryDir withValues;
    const KPackage::Package themed = writeTestTheme(withValues,
                                                    parts
                                                        + "[kdeglobals][Menus]\nOpacity=70\n"
                                                          "[kdeglobals][DesktopThemeOpacity]\nPanels=50\n"
                                                          "[kdeglobals][ToolsArea]\nActiveOpacity=80\n"
                                                          "[silver/silverrc][Windeco]\nButtonIconStyle=StyleArk\n",
                                                    buttons + "ApplicationMenuStyle=AlwaysShown\n");
    QVERIFY(themed.isValid());
    m_lookAndFeel->save(themed, mask);

    KConfig defaults(defaultsDir + u"kdeglobals"_s, KConfig::SimpleConfig);
    QCOMPARE(defaults.group(u"Menus"_s).readEntry("Opacity", 0), 70);
    QCOMPARE(defaults.group(u"DesktopThemeOpacity"_s).readEntry("Panels", 0), 50);
    QCOMPARE(defaults.group(u"ToolsArea"_s).readEntry("ActiveOpacity", 0), 80);
    QCOMPARE(KConfig(defaultsDir + u"silver/silverrc"_s, KConfig::SimpleConfig).group(u"Windeco"_s).readEntry("ButtonIconStyle"), u"StyleArk"_s);
    QCOMPARE(KConfig(defaultsDir + u"kwinrc"_s, KConfig::SimpleConfig).group(u"org.kde.kdecoration2"_s).readEntry("ApplicationMenuStyle"), u"AlwaysShown"_s);
    user->reparseConfiguration();
    QVERIFY(!user->group(u"ToolsArea"_s).hasKey("ActiveOpacity"));

    // A theme without them takes the earlier theme's values away, not the user's own.
    user->group(u"Menus"_s).writeEntry("Opacity", 40);
    user->sync();
    QTemporaryDir withoutValues;
    const KPackage::Package plain = writeTestTheme(withoutValues, parts, buttons);
    QVERIFY(plain.isValid());
    m_lookAndFeel->save(plain, mask);

    defaults.reparseConfiguration();
    QVERIFY(!defaults.group(u"Menus"_s).hasKey("Opacity"));
    QVERIFY(!defaults.group(u"DesktopThemeOpacity"_s).hasKey("Panels"));
    QVERIFY(!defaults.group(u"ToolsArea"_s).hasKey("ActiveOpacity"));
    QVERIFY(!KConfig(defaultsDir + u"silver/silverrc"_s, KConfig::SimpleConfig).group(u"Windeco"_s).hasKey("ButtonIconStyle"));
    QVERIFY(!KConfig(defaultsDir + u"kwinrc"_s, KConfig::SimpleConfig).group(u"org.kde.kdecoration2"_s).hasKey("ApplicationMenuStyle"));
    user->reparseConfiguration();
    QCOMPARE(user->group(u"Menus"_s).readEntry("Opacity", 0), 40);
}

QTEST_MAIN(LookAndFeelManagerTest)
#include "lookandfeelmanagertest.moc"
