add_executable(crashtest crashtest.cpp)
ecm_mark_as_test(crashtest)
ecm_mark_nongui_executable(crashtest)
target_link_libraries(crashtest
    KF5::Crash KF5::CoreAddons Qt5::Gui
    Qt5::Concurrent)

