kcoreaddons_add_plugin(org.kde.plasma.systemmonitor SOURCES systemmonitor.cpp systemmonitor.h INSTALL_NAMESPACE "plasma/applets")

target_link_libraries(org.kde.plasma.systemmonitor
                      Qt::Gui
                      Qt::Qml
                      Qt::Quick
                      Qt::DBus
                      PkgConfig::Sonic
                      PkgConfig::SonicQuick
                      KF6::I18n
                      KF6::ConfigCore
                      KF6::ConfigGui
                      KF6::KIOGui
                      KF6::JobWidgets
                      KSysGuard::Sensors
                      KSysGuard::SensorFaces
                     )

plasma_install_package(package org.kde.plasma.systemmonitor)
