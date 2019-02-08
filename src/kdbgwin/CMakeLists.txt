include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(kdbgwin_SRCS
    common.h
    main.cpp
    process.h
    process.cpp
    abstract_generator.h
    abstract_generator.cpp
    callbacks.h
    callbacks.cpp
    outputters.h
    outputters.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../drkonqi_debug.cpp
)

set(COMMON_LIBS dbghelp psapi shlwapi Qt5::Core)

if ( MINGW )
    add_executable(kdbgwin ${kdbgwin_SRCS} mingw_generator.h mingw_generator.cpp)
    target_link_libraries(kdbgwin PRIVATE ${COMMON_LIBS} bfd iberty intl ZLIB::ZLIB)
endif ()
if ( MSVC OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))
    add_executable(kdbgwin ${kdbgwin_SRCS} msvc_generator.h msvc_generator.cpp)
    target_link_libraries(kdbgwin ${COMMON_LIBS})
endif ()

install(TARGETS kdbgwin DESTINATION ${KDE_INSTALL_LIBEXECDIR})
