kcoreaddons_add_plugin(fontthumbnail SOURCES FontThumbnail.cpp INSTALL_NAMESPACE "kf6/thumbcreator")

ecm_qt_declare_logging_category(fontthumbnail
    HEADER debug.h
    IDENTIFIER KCM_KFONTINST_THUMBNAIL
    CATEGORY_NAME org.kde.kcm_kfontinst.thumbnail
    DESCRIPTION "kcm kfontinst thumbnail"
    EXPORT PLASMAWORKSPACE
)

target_link_libraries(fontthumbnail KF6::Archive KF6::KIOCore KF6::KIOWidgets kfontinstui kfontinst )

if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test)
    add_executable(fontthumbnailtest fontthumbnailtest.cpp)
    target_link_libraries(fontthumbnailtest PRIVATE Qt::Widgets Qt::Test KF6::CoreAddons KF6::KIOGui Fontconfig::Fontconfig)
    target_compile_definitions(fontthumbnailtest PRIVATE
        FONT_THUMBNAIL_PLUGIN="$<TARGET_FILE:fontthumbnail>"
        FONT_THUMBNAIL_LIB_DIR="$<TARGET_FILE_DIR:kfontinstui>")
    add_dependencies(fontthumbnailtest fontthumbnail)
    find_program(XVFB_RUN_EXECUTABLE xvfb-run)
    if(XVFB_RUN_EXECUTABLE)
        add_test(NAME fontthumbnailtest COMMAND ${XVFB_RUN_EXECUTABLE} -a -s "-screen 0 1920x1080x24"
            ${CMAKE_COMMAND} -E env SONIC_FONT_TEST_ISOLATED=1 $<TARGET_FILE:fontthumbnailtest>)
    else()
        add_test(NAME fontthumbnailtest COMMAND fontthumbnailtest)
    endif()
    set_tests_properties(fontthumbnailtest PROPERTIES TIMEOUT 90)
endif()
