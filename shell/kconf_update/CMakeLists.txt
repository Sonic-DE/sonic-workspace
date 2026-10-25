# SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

# Historical shortcut-deletion sources are retained for provenance, but are not
# installed for Sonic Shell. Import must preserve the old component and custom
# bindings, rather than rerun destructive cleanups under a new component name.

add_executable(plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels)
target_sources(plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels PRIVATE plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels.cpp)
target_link_libraries(plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels KF6::ConfigCore)
set_target_properties(plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels PROPERTIES OUTPUT_NAME sonicshell-6.0-keep-default-floating-setting-for-plasma-5-panels)

add_executable(plasmashell-6.0-keep-custom-position-of-panels)
target_sources(plasmashell-6.0-keep-custom-position-of-panels PRIVATE plasmashell-6.0-keep-custom-position-of-panels.cpp)
target_link_libraries(plasmashell-6.0-keep-custom-position-of-panels KF6::ConfigCore)
set_target_properties(plasmashell-6.0-keep-custom-position-of-panels PROPERTIES OUTPUT_NAME sonicshell-6.0-keep-custom-position-of-panels)

install(FILES plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasmashell-6.0-keep-default-floating-setting-for-plasma-5-panels DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)

install(FILES plasmashell-6.0-keep-custom-position-of-panels.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS plasmashell-6.0-keep-custom-position-of-panels DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)

add_executable(sonicde-6.0-repair-missing-theme-components)
target_sources(sonicde-6.0-repair-missing-theme-components PRIVATE sonicde-6.0-repair-missing-theme-components.cpp)
target_link_libraries(sonicde-6.0-repair-missing-theme-components PRIVATE PW::KLookAndFeel KF6::ConfigCore Qt::Core)
install(FILES sonicde-6.0-repair-missing-theme-components.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(TARGETS sonicde-6.0-repair-missing-theme-components DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin/)
