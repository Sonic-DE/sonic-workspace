/*
    SPDX-FileCopyrightText: 2026 SonicDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QString>

namespace SonicShellMigration {
struct Paths {
    QString config;
    QString state;
    QString data;
};

Paths userPaths();
QString layoutFile(const QString &shellPackage);
// No GUI, KSharedConfig, or legacy openStateConfig calls: safe before QApplication.
bool importSettings(const Paths &paths, const QString &shellPackage, QString *error, int lockTimeout = 10000);
// The KDED state-file consumer uses the same lock/import before creating new state.
bool importState(const Paths &paths, QString *error, int lockTimeout = 10000);
}
