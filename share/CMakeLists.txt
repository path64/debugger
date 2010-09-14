
# emacs.el
add_custom_command(OUTPUT "${PATHDB_STAGE_DIR}/share/emacs.el"
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           "${CMAKE_CURRENT_SOURCE_DIR}/pathdb.el"
                           "${PATHDB_STAGE_DIR}/share/emacs.el"
                   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/pathdb.el"
                  )

# Stage
add_custom_target(pathdb-el-stage DEPENDS "${PATHDB_STAGE_DIR}/share/emacs.el")
add_dependencies(pathdb-stage pathdb-el-stage)

# Install
install(FILES "${PATHDB_STAGE_DIR}/share/emacs.el"
        DESTINATION share)

