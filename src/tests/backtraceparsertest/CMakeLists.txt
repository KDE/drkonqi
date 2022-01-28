ecm_add_test(
    fakebacktracegenerator.cpp
    backtraceparsertest.cpp
    TEST_NAME backtraceparsertest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Core KF5::KIOWidgets drkonqi_backtrace_parser
)
add_executable(backtraceparsertest_manual fakebacktracegenerator.cpp backtraceparsertest_manual.cpp)
target_link_libraries(backtraceparsertest_manual Qt${QT_MAJOR_VERSION}::Core KF5::I18n KF5::KIOWidgets drkonqi_backtrace_parser)
ecm_mark_as_test(backtraceparsertest_manual)
ecm_mark_nongui_executable(backtraceparsertest_manual)
