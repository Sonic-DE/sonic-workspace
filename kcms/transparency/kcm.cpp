/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcm.h"

#include "desktopthemeopacitysettings.h"
#include "menusettings.h"
#include "toolsareaexceptionsmodel.h"
#include "toolsareasettings.h"
#include "transparencydata.h"

#include <KConfigGroup>
#include <KDirWatch>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <KSharedConfig>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QQmlEngine>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

K_PLUGIN_FACTORY_WITH_JSON(KCMTransparencyFactory, "kcm_transparency.json", registerPlugin<KCMTransparency>(); registerPlugin<TransparencyData>();)

namespace {
const QString configFiles[] = {u"plasmarc"_s, u"kdeglobals"_s, u"kwinrc"_s};

void sendSignal(const QString &path, const QString &interface, const QString &name, const QVariantList &arguments = {})
{
    QDBusMessage message = QDBusMessage::createSignal(path, interface, name);
    message.setArguments(arguments);
    QDBusConnection::sessionBus().send(message);
}
}

KCMTransparency::KCMTransparency(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new TransparencyData(this))
    , m_exceptions(new ToolsAreaExceptionsModel(KSharedConfig::openConfig(u"kdeglobals"_s), this))
{
    qmlRegisterAnonymousType<DesktopThemeOpacitySettings>("org.sonicde.private.kcms.transparency", 1);
    qmlRegisterAnonymousType<MenuSettings>("org.sonicde.private.kcms.transparency", 1);
    qmlRegisterAnonymousType<ToolsAreaSettings>("org.sonicde.private.kcms.transparency", 1);
    qmlRegisterAnonymousType<QAbstractListModel>("org.sonicde.private.kcms.transparency", 1);

    setButtons(Apply | Default | Help);
    connect(m_exceptions, &ToolsAreaExceptionsModel::changedChanged, this, &KCMTransparency::settingsChanged);

    // Desktop styles and widget styles pick these up at once through KConfigWatcher.
    for (KConfigSkeletonItem *item : desktopStyle()->items() + menus()->items()) {
        item->setWriteFlags(KConfigBase::Notify);
    }

    // The themes can change in other settings pages while this one is open. Those pages
    // save without KConfig's Notify flag, so watch the files themselves.
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    for (const QString &file : configFiles) {
        KDirWatch::self()->addFile(configDir + u'/' + file);
        KDirWatch::self()->addFile(configDir + u"/kdedefaults/"_s + file);
    }
    connect(KDirWatch::self(), &KDirWatch::dirty, this, &KCMTransparency::updateSupport);
    connect(KDirWatch::self(), &KDirWatch::created, this, &KCMTransparency::updateSupport);
    connect(KDirWatch::self(), &KDirWatch::deleted, this, &KCMTransparency::updateSupport);
    updateSupport();
}

KCMTransparency::~KCMTransparency() = default;

DesktopThemeOpacitySettings *KCMTransparency::desktopStyle() const
{
    return m_data->desktopThemeOpacitySettings();
}

MenuSettings *KCMTransparency::menus() const
{
    return m_data->menuSettings();
}

ToolsAreaSettings *KCMTransparency::toolsArea() const
{
    return m_data->toolsAreaSettings();
}

QAbstractListModel *KCMTransparency::toolsAreaExceptions() const
{
    return m_exceptions;
}

bool KCMTransparency::desktopStyleSupported() const
{
    return m_desktopStyleSupported;
}

bool KCMTransparency::applicationStyleSupported() const
{
    return m_applicationStyleSupported;
}

bool KCMTransparency::decorationSupported() const
{
    return m_decorationSupported;
}

void KCMTransparency::updateSupport()
{
    // Desktop style: its colors file opts in (as Desktop Style's themes model reads it).
    // Cascaded: the style can be named only in kdedefaults/ (XDG_CONFIG_DIRS).
    const KConfig plasmarc(u"plasmarc"_s, KConfig::CascadeConfig);
    const QString theme = plasmarc.group(u"Theme"_s).readEntry("name", u"default"_s);
    const QString colors = QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"plasma/desktoptheme/"_s + theme + u"/colors"_s);
    const bool desktopStyle = !colors.isEmpty() && KConfig(colors, KConfig::SimpleConfig).group(u"General"_s).readEntry("FollowDesktopThemeOpacity", false);

    // Application style: Silver and Breeze draw see-through menus.
    const KConfig kdeglobals(u"kdeglobals"_s);
    const QString widgetStyle = kdeglobals.group(u"KDE"_s).readEntry("widgetStyle", u"Breeze"_s);
    const bool applicationStyle = widgetStyle.compare(u"Silver"_s, Qt::CaseInsensitive) == 0 || widgetStyle.compare(u"Breeze"_s, Qt::CaseInsensitive) == 0;

    // Window decoration: its plugin metadata says it draws the see-through tools area.
    const KConfig kwinrc(u"kwinrc"_s, KConfig::CascadeConfig);
    const QString decoration = kwinrc.group(u"org.kde.kdecoration2"_s).readEntry("library", u"org.kde.silver"_s);
    const KPluginMetaData metaData = KPluginMetaData::findPluginById(u"org.kde.kdecoration3"_s, decoration);
    const bool decorationSupport = metaData.isValid() && metaData.rawData().value(u"X-SonicDE-ToolsArea"_s).toBool();

    if (desktopStyle != m_desktopStyleSupported || applicationStyle != m_applicationStyleSupported || decorationSupport != m_decorationSupported) {
        m_desktopStyleSupported = desktopStyle;
        m_applicationStyleSupported = applicationStyle;
        m_decorationSupported = decorationSupport;
        Q_EMIT supportChanged();
    }
}

void KCMTransparency::load()
{
    KQuickManagedConfigModule::load();
    m_exceptions->load();
    updateSupport();
}

void KCMTransparency::save()
{
    // Moving a slider turns its settings on, as the titlebar and desktop style pages did.
    KConfigSkeletonItem *desktopEnabled = desktopStyle()->findItem(u"enabled"_s);
    Q_ASSERT(desktopEnabled);
    if (desktopStyle()->isSaveNeeded() && !desktopEnabled->isSaveNeeded()) {
        desktopStyle()->setEnabled(true);
    }
    const bool exceptionsChanged = m_exceptions->isChanged();
    const bool toolsAreaChanged = toolsArea()->isSaveNeeded() || exceptionsChanged;
    if (toolsArea()->isSaveNeeded()) {
        toolsArea()->setEnabled(true);
    }
    const bool menusChanged = menus()->isSaveNeeded();

    KQuickManagedConfigModule::save();
    if (exceptionsChanged) {
        m_exceptions->save();
    }

    // The window decoration reloads [ToolsArea] with the window manager's settings;
    // applications and widget styles follow [ToolsArea] and [Menus] on notifyChange.
    if (toolsAreaChanged) {
        sendSignal(u"/KWin"_s, u"org.kde.KWin"_s, u"reloadConfig"_s);
    }
    if (toolsAreaChanged || menusChanged) {
        sendSignal(u"/KGlobalSettings"_s, u"org.kde.KGlobalSettings"_s, u"notifyChange"_s, {0, 0});
    }
}

bool KCMTransparency::isSaveNeeded() const
{
    return m_exceptions->isChanged();
}

#include "kcm.moc"
