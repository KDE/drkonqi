set(lib_SRCS
    apijob.cpp
    bugzilla.cpp
    connection.cpp
    exceptions.cpp

    clients/attachmentclient.cpp
    clients/bugclient.cpp
    clients/bugfieldclient.cpp
    clients/commentclient.cpp
    clients/clientbase.cpp
    clients/productclient.cpp

    clients/commands/bugsearch.cpp
    clients/commands/bugupdate.cpp
    clients/commands/jsoncommand.cpp
    clients/commands/newattachment.cpp
    clients/commands/newbug.cpp
    clients/commands/querycommand.cpp

    models/bug.cpp
    models/bugfield.cpp
    models/comment.cpp
    models/logindetails.cpp
    models/product.cpp
)

ecm_qt_declare_logging_category(lib_SRCS
    HEADER bugzilla_debug.h
    IDENTIFIER BUGZILLA_LOG
    CATEGORY_NAME org.kde.drkonqi.bugzilla
    DEFAULT_SEVERITY Warning)

add_library(qbugzilla STATIC ${lib_SRCS})
target_link_libraries(qbugzilla
    PUBLIC
        Qt5::Core
        Qt5::Network
        KF5::CoreAddons
        KF5::KIOCore
)