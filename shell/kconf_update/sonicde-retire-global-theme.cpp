/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Usage: sonicde-retire-global-theme <retired id> <light replacement> <dark replacement>
// Repoints settings naming a removed global theme, kdedefaults/ included, quietly instead
// of through the theme repair's notification.

#include "../../libklookandfeel/themecomponentresolver.h"

#include <QCoreApplication>
#include <QStandardPaths>

#include <KConfig>
#include <KConfigGroup>

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const QStringList args = app.arguments();
    if (args.size() != 4) {
        return 1;
    }
    const QString &retired = args.at(1);
    const QString &light = args.at(2);
    const QString &dark = args.at(3);
    const QString splash = u"org.kde.silverlightbottompanel.desktop"_s;
    const QString switcher = u"thumbnail_grid"_s;

    struct Entry {
        QString file;
        QString group;
        QString key;
        QString replacement;
    };
    const Entry entries[] = {
        {u"kdeglobals"_s, u"KDE"_s, u"LookAndFeelPackage"_s, ThemeComponentResolver::isDarkConfiguration() ? dark : light},
        {u"kdeglobals"_s, u"KDE"_s, u"DefaultLightLookAndFeel"_s, light},
        {u"kdeglobals"_s, u"KDE"_s, u"DefaultDarkLookAndFeel"_s, dark},
        {u"ksplashrc"_s, u"KSplash"_s, u"Theme"_s, splash},
        {u"kwinrc"_s, u"TabBox"_s, u"LayoutName"_s, switcher},
        {u"kwinrc"_s, u"TabBox"_s, u"DesktopLayout"_s, switcher},
    };

    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    for (const QString &dir : {configDir, QString(configDir + u"/kdedefaults"_s)}) {
        for (const Entry &entry : entries) {
            KConfig config(dir + u'/' + entry.file, KConfig::SimpleConfig);
            KConfigGroup group(&config, entry.group);
            if (group.readEntry(entry.key, QString()) == retired) {
                group.writeEntry(entry.key, entry.replacement);
            }
        }
    }
    return 0;
}
