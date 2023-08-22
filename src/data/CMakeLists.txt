install(FILES mappings DESTINATION ${KDE_INSTALL_DATADIR}/drkonqi)
install(DIRECTORY debuggers DESTINATION ${KDE_INSTALL_DATADIR}/drkonqi FILES_MATCHING PATTERN "*rc")
install(DIRECTORY gdb_preamble DESTINATION ${KDE_INSTALL_DATADIR}/drkonqi/gdb/python/)
