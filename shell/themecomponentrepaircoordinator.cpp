/*
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "themecomponentrepaircoordinator.h"

#include "debug.h"
#include "themecomponentresolver.h"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QProcess>
#include <QTimer>
#include <QUrl>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KNotification>
#include <KSharedConfig>

#include <algorithm>
#include <chrono>

namespace {
QString componentName(ThemeComponentResolver::Type type)
{
    switch (type) {
    case ThemeComponentResolver::Type::WidgetStyle:
        return i18n("Widget style");
    case ThemeComponentResolver::Type::ColorScheme:
        return i18n("Color scheme");
    case ThemeComponentResolver::Type::IconTheme:
        return i18n("Icon theme");
    case ThemeComponentResolver::Type::CursorTheme:
        return i18n("Cursor theme");
    case ThemeComponentResolver::Type::PlasmaTheme:
        return i18n("Desktop theme");
    case ThemeComponentResolver::Type::LookAndFeel:
        return i18n("Global theme");
    case ThemeComponentResolver::Type::Decoration:
        return i18n("Window decoration");
    case ThemeComponentResolver::Type::Splash:
        return i18n("Splash screen");
    case ThemeComponentResolver::Type::WindowSwitcher:
        return i18n("Window switcher");
    case ThemeComponentResolver::Type::DesktopSwitcher:
        return i18n("Desktop switcher");
    }
    return {};
}

void writeRepair(const ThemeComponentResolver::Result &repair)
{
    switch (repair.type) {
    case ThemeComponentResolver::Type::WidgetStyle: {
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("KDE"))
            .writeEntry(QStringLiteral("widgetStyle"), repair.replacementId);
        break;
    }
    case ThemeComponentResolver::Type::LookAndFeel: {
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("KDE"))
            .writeEntry(QStringLiteral("LookAndFeelPackage"), repair.replacementId);
        break;
    }
    case ThemeComponentResolver::Type::ColorScheme: {
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("General"))
            .writeEntry(QStringLiteral("ColorScheme"), repair.replacementId);
        break;
    }
    case ThemeComponentResolver::Type::IconTheme: {
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("Icons"))
            .writeEntry(QStringLiteral("Theme"), repair.replacementId);
        break;
    }
    case ThemeComponentResolver::Type::CursorTheme: {
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kcminputrc")), QStringLiteral("Mouse"))
            .writeEntry(QStringLiteral("cursorTheme"), repair.replacementId);
        break;
    }
    case ThemeComponentResolver::Type::PlasmaTheme: {
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Theme")).writeEntry(QStringLiteral("name"), repair.replacementId);
        break;
    }
    case ThemeComponentResolver::Type::Decoration: {
        auto config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
        KConfigGroup group(config, QStringLiteral("org.kde.kdecoration2"));
        group.writeEntry(QStringLiteral("library"), repair.replacementId);
        group.writeEntry(QStringLiteral("theme"), QStringLiteral("Silver"));
        break;
    }
    case ThemeComponentResolver::Type::WindowSwitcher: {
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("TabBox"))
            .writeEntry(QStringLiteral("LayoutName"), repair.replacementId);
        break;
    }
    case ThemeComponentResolver::Type::DesktopSwitcher: {
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("TabBox"))
            .writeEntry(QStringLiteral("DesktopLayout"), repair.replacementId);
        break;
    }
    case ThemeComponentResolver::Type::Splash: {
        KConfigGroup(KSharedConfig::openConfig(QStringLiteral("ksplashrc")), QStringLiteral("KSplash"))
            .writeEntry(QStringLiteral("Theme"), repair.replacementId);
        break;
    }
    }
}
}

ThemeComponentRepairCoordinator::ThemeComponentRepairCoordinator(QObject *parent)
    : QObject(parent)
{
}

void ThemeComponentRepairCoordinator::start()
{
    checkComponents(false);
}

void ThemeComponentRepairCoordinator::checkComponents(bool retry)
{
    const auto repairs = ThemeComponentResolver::resolve(ThemeComponentResolver::isDarkConfiguration());
    if (repairs.isEmpty()) {
        qCDebug(PLASMASHELL) << "All configured theme components are available";
        return;
    }
    if (!retry) {
        qCWarning(PLASMASHELL) << "Theme components unavailable on first check; scheduling bounded retry" << repairs.size();
        QTimer::singleShot(std::chrono::seconds(2), this, [this] {
            checkComponents(true);
        });
        return;
    }

    QStringList details;
    QStringList fingerprintParts;
    bool repairedBreeze = false;
    for (const auto &repair : repairs) {
        writeRepair(repair);
        details.push_back(i18nc("old theme component and replacement", "%1: %2 → %3", componentName(repair.type), repair.configuredId, repair.replacementId));
        fingerprintParts.push_back(QStringLiteral("%1:%2>%3").arg(static_cast<int>(repair.type)).arg(repair.configuredId, repair.replacementId));
        repairedBreeze |= repair.configuredId.contains(QStringLiteral("breeze"), Qt::CaseInsensitive);
        qCWarning(PLASMASHELL) << "Repaired unavailable" << componentName(repair.type) << repair.configuredId << "->" << repair.replacementId;
    }

    KSharedConfig::openConfig(QStringLiteral("kdeglobals"))->sync();
    KSharedConfig::openConfig(QStringLiteral("kcminputrc"))->sync();
    KSharedConfig::openConfig(QStringLiteral("plasmarc"))->sync();
    KSharedConfig::openConfig(QStringLiteral("kwinrc"))->sync();
    KSharedConfig::openConfig(QStringLiteral("ksplashrc"))->sync();

    std::sort(fingerprintParts.begin(), fingerprintParts.end());
    const QString fingerprint =
        QString::fromLatin1(QCryptographicHash::hash(fingerprintParts.join(QLatin1Char('|')).toUtf8(), QCryptographicHash::Sha256).toHex());
    auto stateConfig = KSharedConfig::openConfig(QStringLiteral("plasmashellrc"));
    KConfigGroup state(stateConfig, QStringLiteral("ThemeComponentRepair"));
    if (state.readEntry(QStringLiteral("LastNotificationFingerprint"), QString()) == fingerprint) {
        qCDebug(PLASMASHELL) << "Suppressing repeated theme repair notification" << fingerprint;
        return;
    }
    state.writeEntry(QStringLiteral("LastNotificationFingerprint"), fingerprint);
    state.sync();

    m_notification = KNotification::event(QStringLiteral("themeComponentRepair"),
                                          i18n("Unavailable theme components were replaced"),
                                          details.join(QLatin1Char('\n')),
                                          QStringLiteral("preferences-desktop-theme"),
                                          KNotification::Persistent,
                                          QStringLiteral("plasmashell"));
    if (!m_notification) {
        qCWarning(PLASMASHELL) << "Failed to create theme repair notification";
        return;
    }
    const auto appearanceAction = m_notification->addDefaultAction(i18n("Open Appearance Settings"));
    connect(appearanceAction, &KNotificationAction::activated, this, [] {
        QProcess::startDetached(QStringLiteral("kcmshell6"), {QStringLiteral("kcm_lookandfeel")});
    });
    if (repairedBreeze) {
        const auto breezeAction = m_notification->addAction(i18n("Get Breeze"));
        connect(breezeAction, &KNotificationAction::activated, this, [] {
            if (!QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/Sonic-DE/sonic-breeze")))) {
                qCWarning(PLASMASHELL) << "Failed to request opening the sonic-breeze repository URL";
            }
        });
    }
    m_notification->sendEvent();
}

void ThemeComponentRepairCoordinator::applyRepairs()
{
    checkComponents(true);
}
