/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../../libklookandfeel/themecomponentresolver.h"

#include <QCoreApplication>

#include <KConfigGroup>
#include <KSharedConfig>

namespace {
void writeRepair(const ThemeComponentResolver::Result &repair)
{
    switch (repair.type) {
    case ThemeComponentResolver::Type::WidgetStyle:
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("KDE"))
            .writeEntry(QStringLiteral("widgetStyle"), repair.replacementId);
        break;
    case ThemeComponentResolver::Type::LookAndFeel:
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("KDE"))
            .writeEntry(QStringLiteral("LookAndFeelPackage"), repair.replacementId);
        break;
    case ThemeComponentResolver::Type::ColorScheme:
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("General"))
            .writeEntry(QStringLiteral("ColorScheme"), repair.replacementId);
        break;
    case ThemeComponentResolver::Type::IconTheme:
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("Icons"))
            .writeEntry(QStringLiteral("Theme"), repair.replacementId);
        break;
    case ThemeComponentResolver::Type::CursorTheme:
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kcminputrc")), QStringLiteral("Mouse"))
            .writeEntry(QStringLiteral("cursorTheme"), repair.replacementId);
        break;
    case ThemeComponentResolver::Type::PlasmaTheme:
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Theme")).writeEntry(QStringLiteral("name"), repair.replacementId);
        break;
    case ThemeComponentResolver::Type::Decoration: {
        KConfigGroup group(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("org.kde.kdecoration2"));
        group.writeEntry(QStringLiteral("library"), repair.replacementId);
        group.writeEntry(QStringLiteral("theme"), QStringLiteral("Silver"));
        break;
    }
    case ThemeComponentResolver::Type::Splash:
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("ksplashrc")), QStringLiteral("KSplash"))
            .writeEntry(QStringLiteral("Theme"), repair.replacementId);
        break;
    case ThemeComponentResolver::Type::WindowSwitcher:
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("TabBox"))
            .writeEntry(QStringLiteral("LayoutName"), repair.replacementId);
        break;
    case ThemeComponentResolver::Type::DesktopSwitcher:
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("TabBox"))
            .writeEntry(QStringLiteral("DesktopLayout"), repair.replacementId);
        break;
    }
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto repairs = ThemeComponentResolver::resolve(ThemeComponentResolver::isDarkConfiguration());
    for (const auto &repair : repairs) {
        writeRepair(repair);
    }
    for (const QString &file :
         {QStringLiteral("kdeglobals"), QStringLiteral("kcminputrc"), QStringLiteral("plasmarc"), QStringLiteral("kwinrc"), QStringLiteral("ksplashrc")}) {
        KSharedConfig::openConfig(file)->sync();
    }
    return 0;
}
