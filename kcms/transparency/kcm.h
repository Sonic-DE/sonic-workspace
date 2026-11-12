/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KQuickManagedConfigModule>

#include <QAbstractListModel>

#include "desktopthemeopacitysettings.h"
#include "menusettings.h"
#include "toolsareasettings.h"

class ToolsAreaExceptionsModel;
class TransparencyData;

// Every transparency setting in one page: desktop style ([DesktopThemeOpacity]), menus
// ([Menus]) and titlebars ([ToolsArea] and its exceptions), all in kdeglobals.
class KCMTransparency : public KQuickManagedConfigModule {
    Q_OBJECT

    Q_PROPERTY(DesktopThemeOpacitySettings *desktopStyle READ desktopStyle CONSTANT)
    Q_PROPERTY(MenuSettings *menus READ menus CONSTANT)
    Q_PROPERTY(ToolsAreaSettings *toolsArea READ toolsArea CONSTANT)
    Q_PROPERTY(QAbstractListModel *toolsAreaExceptions READ toolsAreaExceptions CONSTANT)

    // Whether the selected desktop style, application style and window decoration use these settings.
    Q_PROPERTY(bool desktopStyleSupported READ desktopStyleSupported NOTIFY supportChanged)
    Q_PROPERTY(bool applicationStyleSupported READ applicationStyleSupported NOTIFY supportChanged)
    Q_PROPERTY(bool decorationSupported READ decorationSupported NOTIFY supportChanged)

public:
    KCMTransparency(QObject *parent, const KPluginMetaData &data);
    ~KCMTransparency() override;

    DesktopThemeOpacitySettings *desktopStyle() const;
    MenuSettings *menus() const;
    ToolsAreaSettings *toolsArea() const;
    QAbstractListModel *toolsAreaExceptions() const;

    bool desktopStyleSupported() const;
    bool applicationStyleSupported() const;
    bool decorationSupported() const;

Q_SIGNALS:
    void supportChanged();

public Q_SLOTS:
    void load() override;
    void save() override;

private:
    bool isSaveNeeded() const override;
    void updateSupport();

    TransparencyData *const m_data;
    ToolsAreaExceptionsModel *const m_exceptions;
    bool m_desktopStyleSupported = false;
    bool m_applicationStyleSupported = false;
    bool m_decorationSupported = false;
};
