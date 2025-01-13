remove_definitions(-DQT_NO_CAST_FROM_ASCII)

include(ECMAddTests)
include(GenerateExportHeader)

# Include src so we have access to config-kcrash.h
include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ecm_add_tests(
    attachmenttest.cpp
    bugtest.cpp
    bugfieldtest.cpp
    bugzillatest.cpp
    commenttest.cpp
    connectiontest.cpp
    producttest.cpp
    LINK_LIBRARIES Qt::Core Qt::Test Qt::Network qbugzilla
)

ecm_mark_nongui_executable(bugzillatest)
