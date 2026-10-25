include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(sonicshellmigrationtest legacyshellmigrationtest.cpp)
target_link_libraries(sonicshellmigrationtest PRIVATE sonicshellmigration Qt::Test)
add_test(NAME sonicshellmigrationtest COMMAND sonicshellmigrationtest)
