/*
    SPDX-FileCopyrightText: 2026 SonicDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

inline bool sonicShellImportCompleted()
{
    QFile record(QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) + QStringLiteral("/.sonicshell-base-import.json"));
    return record.open(QIODevice::ReadOnly) && QJsonDocument::fromJson(record.readAll()).object().value(QStringLiteral("version")).toInt() == 1;
}
