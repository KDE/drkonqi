add_executable(crashtest crashtest.cpp)
ecm_mark_as_test(crashtest)
ecm_mark_nongui_executable(crashtest)
target_link_libraries(crashtest KF5::Crash KF5::CoreAddons Qt${QT_MAJOR_VERSION}::Gui Qt${QT_MAJOR_VERSION}::Concurrent)
