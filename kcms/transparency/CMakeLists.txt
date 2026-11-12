# SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_definitions(-DTRANSLATION_DOMAIN=\"kcm_transparency\")

kcmutils_add_qml_kcm(kcm_transparency)

target_sources(kcm_transparency PRIVATE
    kcm.cpp
    kcm.h
    toolsareaexceptionsmodel.cpp
    toolsareaexceptionsmodel.h
)
kcmutils_generate_module_data(kcm_transparency
    MODULE_DATA_HEADER transparencydata.h
    MODULE_DATA_CLASS_NAME TransparencyData
    SETTINGS_HEADERS desktopthemeopacitysettings.h toolsareasettings.h menusettings.h
    SETTINGS_CLASSES DesktopThemeOpacitySettings ToolsAreaSettings MenuSettings
)
kconfig_add_kcfg_files(kcm_transparency desktopthemeopacitysettings.kcfgc toolsareasettings.kcfgc menusettings.kcfgc GENERATE_MOC)

target_link_libraries(kcm_transparency PRIVATE
    Qt::DBus
    Qt::Quick
    KF6::ColorScheme
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtils
    KF6::KCMUtilsQuick
)
