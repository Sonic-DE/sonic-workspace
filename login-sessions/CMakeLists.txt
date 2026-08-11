## For SonicDE end users

configure_file(sonicde.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/sonicde.desktop)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/sonicde.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/xsessions
)

## For SonicDE developers
configure_file(startplasma-dev.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/startplasma-dev.sh)
configure_file(plasmax11-dev.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasmax11-dev6.desktop)
configure_file(install-sessions.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/install-sessions.sh)
