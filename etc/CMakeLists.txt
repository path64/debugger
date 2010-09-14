
# Help system
add_custom_command(OUTPUT "${PATHDB_STAGE_DIR}/etc/pathdb-help.xml"
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           "${CMAKE_CURRENT_SOURCE_DIR}/help.xml"
                           "${PATHDB_STAGE_DIR}/etc/pathdb-help.xml"
                   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/help.xml"
                  )

# Stage
add_custom_target(pathdb-help-stage DEPENDS "${PATHDB_STAGE_DIR}/etc/pathdb-help.xml")
add_dependencies(pathdb-stage pathdb-help-stage)

# Install
install(FILES "${PATHDB_STAGE_DIR}/etc/pathdb-help.xml"
        DESTINATION etc)

