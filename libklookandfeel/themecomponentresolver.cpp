/*
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "themecomponentresolver.h"

#include <QDir>
#include <QSet>
#include <QStandardPaths>
#include <QStyleFactory>

#include <KConfig>
#include <KConfigGroup>
#include <KIconTheme>
#include <KPluginMetaData>
#include <KSharedConfig>

#include <algorithm>

namespace {
bool dataDirectoryExists(const QString &relativePath)
{
    return !QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, relativePath, QStandardPaths::LocateDirectory).isEmpty();
}

bool dataFileExists(const QString &relativePath)
{
    return !QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, relativePath, QStandardPaths::LocateFile).isEmpty();
}

bool cursorThemeExists(const QString &id, QSet<QString> *visited = nullptr, int depth = 0)
{
    if (depth > 16) {
        return false;
    }
    QSet<QString> localVisited;
    if (!visited) {
        visited = &localVisited;
    }
    if (visited->contains(id)) {
        return false;
    }
    visited->insert(id);
    const auto roots = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("icons/") + id, QStandardPaths::LocateDirectory);
    for (const QString &root : roots) {
        if (QDir(root + QStringLiteral("/cursors")).exists()) {
            return true;
        }
        KConfig theme(root + QStringLiteral("/index.theme"), KConfig::SimpleConfig);
        const QStringList inherited = KConfigGroup(&theme, QStringLiteral("Icon Theme")).readEntry(QStringLiteral("Inherits"), QStringList());
        for (const QString &parent : inherited) {
            if (cursorThemeExists(parent, visited, depth + 1)) {
                return true;
            }
        }
    }
    return false;
}

bool styleExists(const QString &name)
{
    const auto keys = QStyleFactory::keys();
    return std::any_of(keys.cbegin(), keys.cend(), [&name](const QString &key) {
        return key.compare(name, Qt::CaseInsensitive) == 0;
    });
}

bool decorationExists(const QString &id)
{
    const auto plugins = KPluginMetaData::findPlugins(QStringLiteral("org.kde.kdecoration3"));
    return std::any_of(plugins.cbegin(), plugins.cend(), [&id](const KPluginMetaData &metadata) {
        return metadata.pluginId() == id || metadata.fileName().contains(id);
    });
}

void appendResult(QVector<ThemeComponentResolver::Result> &results, ThemeComponentResolver::Type type, const QString &configured, bool dark)
{
    if (!configured.isEmpty() && !ThemeComponentResolver::isAvailable(type, configured)) {
        results.push_back({type, configured, ThemeComponentResolver::fallback(type, dark), false});
    }
}
}

bool ThemeComponentResolver::isAvailable(Type type, const QString &id)
{
    if (id.isEmpty()) {
        return true;
    }
    switch (type) {
    case Type::WidgetStyle:
        return styleExists(id);
    case Type::ColorScheme:
        return dataFileExists(QStringLiteral("color-schemes/") + id + QStringLiteral(".colors"));
    case Type::IconTheme:
        return KIconTheme(id).isValid();
    case Type::CursorTheme:
        return cursorThemeExists(id);
    case Type::PlasmaTheme:
        return dataDirectoryExists(QStringLiteral("plasma/desktoptheme/") + id);
    case Type::LookAndFeel:
        return dataDirectoryExists(QStringLiteral("plasma/look-and-feel/") + id);
    case Type::Decoration:
        return decorationExists(id);
    case Type::Splash:
        return dataDirectoryExists(QStringLiteral("plasma/look-and-feel/") + id + QStringLiteral("/contents/splash"));
    case Type::WindowSwitcher:
    case Type::DesktopSwitcher:
        // Switcher layouts may be supplied by a look-and-feel package or a
        // dedicated tabbox package. Either is a valid third-party selection.
        return dataDirectoryExists(QStringLiteral("plasma/look-and-feel/") + id) || dataDirectoryExists(QStringLiteral("kwin/tabbox/") + id);
    }
    return false;
}

QString ThemeComponentResolver::fallback(Type type, bool dark)
{
    switch (type) {
    case Type::WidgetStyle:
        return QStringLiteral("Silver");
    case Type::ColorScheme:
        return dark ? QStringLiteral("SilverDark") : QStringLiteral("SilverLight");
    case Type::IconTheme:
        return dark ? QStringLiteral("silver-dark") : QStringLiteral("silver");
    case Type::CursorTheme:
        return dark ? QStringLiteral("silver_cursors_dark") : QStringLiteral("silver_cursors_light");
    case Type::PlasmaTheme:
        return dark ? QStringLiteral("silver-dark") : QStringLiteral("silver-light");
    case Type::LookAndFeel:
        return dark ? QStringLiteral("org.kde.silverdarkbottompanel.desktop") : QStringLiteral("org.kde.silverlightbottompanel.desktop");
    case Type::Decoration:
        return QStringLiteral("org.kde.silver");
    case Type::Splash:
    case Type::WindowSwitcher:
    case Type::DesktopSwitcher:
        return QStringLiteral("org.kde.silver.desktop");
    }
    return {};
}

bool ThemeComponentResolver::isDarkConfiguration()
{
    KConfigGroup general(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("General"));
    const QString colorScheme = general.readEntry(QStringLiteral("ColorScheme"), QString());
    return colorScheme.contains(QStringLiteral("dark"), Qt::CaseInsensitive);
}

QVector<ThemeComponentResolver::Result> ThemeComponentResolver::resolve(bool dark)
{
    QVector<Result> results;
    const auto globals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KConfigGroup kde(globals, QStringLiteral("KDE"));
    KConfigGroup general(globals, QStringLiteral("General"));
    KConfigGroup icons(globals, QStringLiteral("Icons"));
    appendResult(results, Type::WidgetStyle, kde.readEntry(QStringLiteral("widgetStyle"), QString()), dark);
    appendResult(results, Type::LookAndFeel, kde.readEntry(QStringLiteral("LookAndFeelPackage"), QString()), dark);
    appendResult(results, Type::ColorScheme, general.readEntry(QStringLiteral("ColorScheme"), QString()), dark);
    appendResult(results, Type::IconTheme, icons.readEntry(QStringLiteral("Theme"), QString()), dark);

    KConfigGroup mouse(KSharedConfig::openConfig(QStringLiteral("kcminputrc")), QStringLiteral("Mouse"));
    appendResult(results, Type::CursorTheme, mouse.readEntry(QStringLiteral("cursorTheme"), QString()), dark);

    KConfigGroup plasma(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Theme"));
    appendResult(results, Type::PlasmaTheme, plasma.readEntry(QStringLiteral("name"), QString()), dark);

    const auto kwin = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
    KConfigGroup decoration(kwin, QStringLiteral("org.kde.kdecoration2"));
    appendResult(results, Type::Decoration, decoration.readEntry(QStringLiteral("library"), QString()), dark);
    KConfigGroup tabbox(kwin, QStringLiteral("TabBox"));
    appendResult(results, Type::WindowSwitcher, tabbox.readEntry(QStringLiteral("LayoutName"), QString()), dark);
    appendResult(results, Type::DesktopSwitcher, tabbox.readEntry(QStringLiteral("DesktopLayout"), QString()), dark);

    KConfigGroup splash(KSharedConfig::openConfig(QStringLiteral("ksplashrc")), QStringLiteral("KSplash"));
    appendResult(results, Type::Splash, splash.readEntry(QStringLiteral("Theme"), QString()), dark);
    return results;
}
