/*
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "klookandfeel_export.h"

#include <QString>
#include <QVector>

class KLOOKANDFEEL_EXPORT ThemeComponentResolver {
public:
    enum class Type {
        WidgetStyle,
        ColorScheme,
        IconTheme,
        CursorTheme,
        PlasmaTheme,
        LookAndFeel,
        Decoration,
        Splash,
        WindowSwitcher,
        DesktopSwitcher,
    };

    struct Result {
        Type type;
        QString configuredId;
        QString replacementId;
        bool available = true;
    };

    static bool isAvailable(Type type, const QString &id);
    static QString fallback(Type type, bool dark);
    static QVector<Result> resolve(bool dark);
    static bool isDarkConfiguration();
};
