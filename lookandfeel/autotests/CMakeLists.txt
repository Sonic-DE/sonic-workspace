find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)
add_executable(wallpaperfadertest wallpaperfadertest.cpp)
target_link_libraries(wallpaperfadertest PRIVATE Qt::Quick Qt::Test)
target_compile_definitions(wallpaperfadertest PRIVATE
    WALLPAPER_FADER_TEST_QML="${CMAKE_CURRENT_SOURCE_DIR}/WallpaperFaderTest.qml")
qt6_add_shaders(wallpaperfadertest WallpaperFaderTestShaders
    PREFIX "/qt/qml/org/kde/breeze/components/shaders/"
    FILES ../components/WallpaperFader.frag OUTPUTS WallpaperFader.frag.qsb)
find_program(XVFB_RUN_EXECUTABLE xvfb-run)
if(XVFB_RUN_EXECUTABLE)
    add_test(NAME wallpaperfadertest COMMAND ${XVFB_RUN_EXECUTABLE} -a -s "-screen 0 1280x900x24"
        ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=xcb QSG_RHI_BACKEND=opengl
        LIBGL_ALWAYS_SOFTWARE=1 SONIC_WALLPAPER_TEST_ISOLATED=1 $<TARGET_FILE:wallpaperfadertest>)
else()
    add_test(NAME wallpaperfadertest COMMAND wallpaperfadertest)
    set_tests_properties(wallpaperfadertest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
set_tests_properties(wallpaperfadertest PROPERTIES TIMEOUT 90)
