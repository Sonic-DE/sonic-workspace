# SPDX-License-Identifier: BSD-2-Clause
#
# SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

ecm_add_qml_module(loginlockscreen
    URI "org.kde.plasma.workspace.loginlockscreen"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
)

find_package(Qt6VirtualKeyboard REQUIRED)
find_package(Qt6GuiPrivate REQUIRED)
target_sources(loginlockscreen PRIVATE virtualkeyboardcontroller.cpp virtualkeyboardcontroller.h)
target_link_libraries(loginlockscreen PRIVATE Qt6::VirtualKeyboard Qt6::GuiPrivate)

ecm_target_qml_sources(loginlockscreen
    SOURCES
    Footer.qml
)

ecm_finalize_qml_module(loginlockscreen)

if(BUILD_TESTING)
    add_executable(virtualkeyboardcontrollertest autotests/virtualkeyboardcontrollertest.cpp virtualkeyboardcontroller.cpp)
    target_include_directories(virtualkeyboardcontrollertest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(virtualkeyboardcontrollertest PRIVATE Qt6::Test Qt6::Qml Qt6::GuiPrivate)
    add_test(NAME virtualkeyboardcontrollertest COMMAND virtualkeyboardcontrollertest)
endif()
