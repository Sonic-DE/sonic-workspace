if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()

set(taskmanager_LIB_SRCS
    abstracttasksmodel.cpp
    abstracttasksproxymodeliface.cpp
    abstractwindowtasksmodel.cpp
    activityinfo.cpp
    concatenatetasksproxymodel.cpp
    flattentaskgroupsproxymodel.cpp
    launchertasksmodel.cpp
    startuptasksmodel.cpp
    regionfiltermode.h
    taskfilterproxymodel.cpp
    taskgroupingproxymodel.cpp
    tasksmodel.cpp
    tasktools.cpp
    virtualdesktopinfo.cpp
    windowtasksmodel.cpp
)

set(taskmanager_LIB_SRCS
        ${taskmanager_LIB_SRCS}
        xstartuptasksmodel.cpp
        xwindowsystemeventbatcher.cpp xwindowsystemeventbatcher.h
        xwindowtasksmodel.cpp
)

add_library(taskmanager ${taskmanager_LIB_SRCS})
add_library(PW::LibTaskManager ALIAS taskmanager)

ecm_add_qml_module(taskmanager
    URI "org.kde.taskmanager"
    VERSION 0.1
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES QtCore
)

if (Qt6_VERSION VERSION_GREATER_EQUAL "6.8.0")
    set(private_code_option  "PRIVATE_CODE")
endif()

generate_export_header(taskmanager)

target_include_directories(taskmanager PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/taskmanager>")

# Optionally use Qt::Test if available to find a crash
# This can be reverted before release hopefully
find_package(Qt6Test ${QT_MIN_VERSION} CONFIG QUIET)

target_link_libraries(taskmanager
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Quick
        KF6::ItemModels
    PRIVATE
        Qt::Concurrent
        Qt::DBus
        Qt::GuiPrivate
        Plasma::Activities
        KF6::ConfigCore
        KF6::I18n
        KF6::KIOCore
        KF6::KIOGui
        KF6::JobWidgets
        KF6::Service
        KF6::WindowSystem
        PW::KWorkspace
)

# Define and link Qt::Test only if available
if(Qt6Test_FOUND)
    target_compile_definitions(taskmanager PRIVATE HAVE_QTTEST=1)
    target_link_libraries(taskmanager PRIVATE Qt::Test)
else()
    target_compile_definitions(taskmanager PRIVATE HAVE_QTTEST=0)
endif()

target_link_libraries(taskmanager PRIVATE KF6::IconThemes)
target_link_libraries(taskmanager PRIVATE X11::X11)

set_target_properties(taskmanager PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION 6
                      EXPORT_NAME LibTaskManager)


ecm_qt_declare_logging_category(taskmanager
    HEADER libtaskmanager_debug.h
    IDENTIFIER TASKMANAGER_DEBUG
    CATEGORY_NAME org.kde.plasma.libtaskmanager
    DESCRIPTION "libtaskmanager debug"
    EXPORT PLASMAWORKSPACE
)

install(TARGETS taskmanager EXPORT libtaskmanagerLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES
    abstracttasksmodel.h
    abstracttasksmodeliface.h
    abstracttasksproxymodeliface.h
    abstractwindowtasksmodel.h
    activityinfo.h
    concatenatetasksproxymodel.h
    flattentaskgroupsproxymodel.h
    launchertasksmodel.h
    regionfiltermode.h
    startuptasksmodel.h
    taskfilterproxymodel.h
    taskgroupingproxymodel.h
    tasksmodel.h
    tasktools.h
    virtualdesktopinfo.h
    windowtasksmodel.h
    ${CMAKE_CURRENT_BINARY_DIR}/taskmanager_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/taskmanager COMPONENT Devel
)

install(FILES
        xstartuptasksmodel.h
        xwindowtasksmodel.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/taskmanager COMPONENT Devel
)

ecm_finalize_qml_module(taskmanager)

write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfigVersion.cmake VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion)

set(CMAKECONFIG_INSTALL_DIR ${KDE_INSTALL_CMAKEPACKAGEDIR}/LibTaskManager)

configure_package_config_file(LibTaskManagerConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/LibTaskManagerConfigVersion.cmake
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(EXPORT libtaskmanagerLibraryTargets
        NAMESPACE PW::
        DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        FILE LibTaskManagerLibraryTargets.cmake )
