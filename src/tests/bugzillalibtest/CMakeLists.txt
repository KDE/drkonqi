set(bugzillalibtest_SRCS bugzillalibtest.cpp ../../bugzillaintegration/bugzillalib.cpp)

ecm_qt_declare_logging_category(
    bugzillalibtest_SRCS
    HEADER drkonqi_debug.h
    IDENTIFIER DRKONQI_LOG
    CATEGORY_NAME org.kde.drkonqi
)

add_executable(bugzillalibtest ${bugzillalibtest_SRCS})
ecm_mark_as_test(bugzillalibtest)

target_link_libraries(bugzillalibtest KF5::KIOWidgets KF5::I18n qbugzilla)
