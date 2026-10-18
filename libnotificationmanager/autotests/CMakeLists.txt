include(ECMMarkAsTest)

set(notifications_test_SRCS
    notifications_test.cpp
)
add_executable(notification_test  ${notifications_test_SRCS})
target_link_libraries(notification_test Qt::Test Qt::Core PW::LibNotificationManager)
add_test(NAME libnotificationmanager-test COMMAND notification_test)
ecm_mark_as_test(notification_test)

add_executable(notification_settings_test settings_test.cpp)
target_link_libraries(notification_settings_test Qt::Test KF6::ConfigGui PW::LibNotificationManager)
add_test(NAME libnotificationmanager-settings-test COMMAND notification_settings_test)
ecm_mark_as_test(notification_settings_test)
