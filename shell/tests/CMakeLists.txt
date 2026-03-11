
include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(screenpoolmanualtest_SRCS
    screenpooltest.cpp
    ../screenpool.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../screenpool-debug.cpp
    )
add_executable(screenpoolmanualtest)

target_sources(screenpoolmanualtest PRIVATE ${screenpoolmanualtest_SRCS})
target_link_libraries(screenpoolmanualtest
            Qt::Test
            Qt::Gui
            Qt::Widgets
            KF6::ConfigCore
            KF6::Service
            KF6::WindowSystem
            PW::KWorkspace
                        )
target_link_libraries(screenpoolmanualtest XCB::XCB XCB::RANDR)
target_link_libraries(screenpoolmanualtest Qt::GuiPrivate)
if(QT_QTOPENGL_FOUND)
    target_link_libraries(screenpoolmanualtest Qt::OpenGL)
endif()


set(setwallpapertest_SRCS
    setwallpapertest.cpp
    )
add_executable(setwallpapertest)

target_sources(setwallpapertest PRIVATE ${setwallpapertest_SRCS})
target_link_libraries(setwallpapertest
            Qt::Test
            Qt::DBus
    )
