set(PLASMA_RELATIVE_DATA_INSTALL_DIR "plasma")
set(PLASMA_DATA_INSTALL_DIR "${KDE_INSTALL_DATADIR}/${PLASMA_RELATIVE_DATA_INSTALL_DIR}")

# plasma_install_package(path componentname [root] [type])
#
# Installs a Plasma package to the system path
# @arg path The source path to install from, location of metadata.desktop
# @arg componentname The plugin name of the component, corresponding to the
#       X-KDE-PluginInfo-Name key in metadata.desktop
# @arg root The subdirectory to install to, default: plasmoids
# @arg type The type, default to applet, or applet, package, containment,
#       wallpaper, shell, lookandfeel, etc.
# @see Types column in kpackagetool6 --list-types
#
# Examples:
# plasma_install_package(mywidget org.kde.plasma.mywidget) # installs an applet
# plasma_install_package(declarativetoolbox org.kde.toolbox packages package) # installs a generic package
#
macro(plasma_install_package dir component)
   set(root ${ARGV2})
   set(type ${ARGV3})
   if(NOT root)
      set(root plasmoids)
   endif()
   if(NOT type)
      set(type applet)
   endif()

   kpackage_install_package(${dir} ${component} ${root} ${PLASMA_RELATIVE_DATA_INSTALL_DIR} NO_DEPRECATED_WARNING)
endmacro()

# plasma_add_applet(id QML_SOURCES ... [CPP_SOURCES] [RESOURCES])
#
# Creates a plasma applet.
#
# An applet consists of one or more QML files and optionally C++ sources.
#
# QML_SOURCES: A list of QML files for this applet
# CPP_SOURCES: A list of C++ sources for this applet
# RESOURCES: A list of files to be added to the applet's QRC
# GENERATE_APPLET_CLASS: Pass this to automatically generate a Plasma::Applet subclass if no non-trivial class is needed
# GENERATE_CONTAINMENT_CLASS: Pass this to automatically generate a Plasma::Containment subclass if no non-trivial class is needed.
# QML_ARGS: These will be forwarded as arguments to the interal qt_add_qml_module call. See https://doc.qt.io/qt-6/qt-add-qml-module.html for available arguments
#
# This creates a CMake target named after the given id which can be manipulated further using CMake API
#
# Example:
# plasma_add_applet(org.kde.plasma.calculator
#     QML_SOURCES main.qml
#     CPP_SOURCES calculator.cpp
#     GENERATE_APPLET_CLASS
# )
#
function(plasma_add_applet id)
   set(options GENERATE_APPLET_CLASS GENERATE_CONTAINMENT_CLASS)
   set(oneValueArgs)
   set(multiValueArgs QML_SOURCES CPP_SOURCES RESOURCES QML_ARGS)
   cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

   # not using MODULE because of https://bugreports.qt.io/browse/QTBUG-117159
   add_library(${id} SHARED)

   set_target_properties(${id} PROPERTIES PREFIX "")

   include(ECMQmlModule)
   ecm_add_qml_module(${id} URI "plasma.applet.${id}" QT_NO_PLUGIN ${ARGS_QML_ARGS})

   ecm_target_qml_sources(${id} SOURCES ${ARGS_QML_SOURCES} RESOURCES ${ARGS_RESOURCES})

   set_target_properties(${id} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/plasma/applets")

   if(ARGS_GENERATE_APPLET_CLASS OR ARGS_GENERATE_CONTAINMENT_CLASS)

      string(REPLACE "." "_" CLASS_NAME ${id})
      string(REPLACE "-" "_" CLASS_NAME ${CLASS_NAME})

      if (ARGS_GENERATE_APPLET_CLASS)
          set(CLASS_TYPE "Applet")
      elseif (ARGS_GENERATE_CONTAINMENT_CLASS)
          set(CLASS_TYPE "Containment")
      endif()

      set(PLUGIN_SRC
"
#include <KPluginFactory>
#include <Plasma/${CLASS_TYPE}>

class ${CLASS_NAME}_Plugin : public Plasma::${CLASS_TYPE} {
   Q_OBJECT
public:
   ${CLASS_NAME}_Plugin(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
      : Plasma::${CLASS_TYPE}(parent, data, args) {}
}\;

K_PLUGIN_CLASS_WITH_JSON(${CLASS_NAME}_Plugin, \"metadata.json\")

#include \"${id}.moc\"
")

      file(GENERATE OUTPUT ${id}.cpp CONTENT ${PLUGIN_SRC})

      target_sources(${id} PRIVATE ${id}.cpp)

      target_link_libraries(${id} PRIVATE KF6::CoreAddons Plasma::Plasma)
   endif()

   target_sources(${id} PRIVATE ${ARGS_CPP_SOURCES})

   if (NOT PLASMA_SKIP_APPLET_INSTALL)
      install(TARGETS ${id} DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/applets)
   endif()
endfunction()
