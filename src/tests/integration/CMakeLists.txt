add_test(
    NAME drkonqi_integration_suite
    COMMAND
        ${RUBY_EXECTUABLE} ${CMAKE_CURRENT_SOURCE_DIR}/suite --drkonqi $<TARGET_FILE:drkonqi> --at-spi-bus-launcher
        ${ATSPI_BUS_LAUNCHER_EXECUTABLE} --at-spi-registryd ${ATSPI_REGISTRYD_EXECUTABLE}
)
set_tests_properties(drkonqi_integration_suite PROPERTIES TIMEOUT 150)

# Hack to get rb files to show in qtc.
file(GLOB RUBIES suite *.rb)
add_custom_target(Rubies ALL echo SOURCES ${RUBIES})
