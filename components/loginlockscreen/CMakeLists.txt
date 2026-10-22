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
    add_executable(virtualkeyboardbackendtest
        autotests/virtualkeyboardbackendtest.cpp virtualkeyboardcontroller.cpp virtualkeyboardcontroller.h)
    target_include_directories(virtualkeyboardbackendtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(virtualkeyboardbackendtest PRIVATE Qt::Test Qt::Quick Qt::GuiPrivate)
    find_program(XVFB_RUN_EXECUTABLE xvfb-run)
    if(XVFB_RUN_EXECUTABLE)
        add_test(NAME virtualkeyboardbackendtest COMMAND ${XVFB_RUN_EXECUTABLE} -a -s "-screen 0 1280x900x24"
            ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=xcb SONIC_KEYBOARD_TEST_ISOLATED=1
            $<TARGET_FILE:virtualkeyboardbackendtest>)
    else()
        add_test(NAME virtualkeyboardbackendtest COMMAND virtualkeyboardbackendtest)
        set_tests_properties(virtualkeyboardbackendtest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
    set_tests_properties(virtualkeyboardbackendtest PROPERTIES TIMEOUT 60)
endif()
