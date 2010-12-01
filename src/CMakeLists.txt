

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PATHDB_STAGE_DIR}/bin")

set(pathdb_sources
    dbg_thread_db.cc
    dbg_proc_service.cc
    pcm.cc
    breakpoint.cc
    process.cc
    thread.cc
    arch.cc
    symtab.cc
    expr.cc
    dbg_dwarf.cc
    dbg_elf.cc
    dis.cc
    cli.cc
    pstream.cc
    target.cc
    readline.cc
    dbg_except.cc
    utils.cc
    dbg_stl.cc
    type_struct.cc
    type_union.cc
    type_base.cc
    cli_param.cc
    cli_history.cc
    dwf_cfa.cc
    bstream.cc
    xml.cc
    type_array.cc
    type_class.cc
    type_enum.cc
    type_pointer.cc
    type_nspace.cc
    dwf_info.cc
    dwf_locs.cc
    dwf_names.cc
    dwf_cunit.cc
    file_info.cc
    type_qual.cc
    dwf_entry.cc
    dwf_abbrv.cc
    gen_loc.cc
    junk_stream.cc
    ${CMAKE_CURRENT_BINARY_DIR}/opcodes.cc
    ${CMAKE_CURRENT_BINARY_DIR}/version.cc
    ${CMAKE_CURRENT_BINARY_DIR}/dwf_spec.cc
    register_set.cc
    ptrace_target.cc
    x86_arch.cc
    x86registers.cc
    os.cc
    trace.cc
)

# Getting current date/time
execute_process(COMMAND date +%F/%T
                OUTPUT_VARIABLE curr_date
                OUTPUT_STRIP_TRAILING_WHITESPACE)

add_definitions(-DPSC_BUILD_DATE=\"${curr_date}\")

# Configuring version.cc
set(PATHDB_REV "unknown")   # TODO
set(PATHDB_KEY "unknown")   # TODO
get_filename_component(PATHDB_BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}/.." ABSOLUTE)
execute_process(COMMAND hostname -f
                OUTPUT_VARIABLE PATHDB_BUILD_HOST
                OUTPUT_STRIP_TRAILING_WHITESPACE)
set(PATHDB_BUILD_DATE ${curr_date})
if(PSC_FULL_VERSION)
    set(PATHDB_FULL_VER "${PSC_FULL_VERSION}")
else()
    set(PATHDB_FULL_VER "unknown")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.cc.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/version.cc")

# Generating opcodes.cc
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/opcodes.cc"
                   COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/mkopcodes.py"
                           "${CMAKE_CURRENT_SOURCE_DIR}/opcodes.txt" >
                           "${CMAKE_CURRENT_BINARY_DIR}/opcodes.cc"
                   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mkopcodes.py"
                           "${CMAKE_CURRENT_SOURCE_DIR}/opcodes.txt"
                  )

# Generating dwf_spec.h
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dwf_spec.h"
                   COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/mkdwfspec.py"
                           "${CMAKE_CURRENT_SOURCE_DIR}/dwf_spec.txt" >
                           "${CMAKE_CURRENT_BINARY_DIR}/dwf_spec.h"
                   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mkdwfspec.py"
                           "${CMAKE_CURRENT_SOURCE_DIR}/dwf_spec.txt"
                  )

# Generating dwf_spec.cc
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dwf_spec.cc"
                   COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/mkdwfspec.py" -d
                           "${CMAKE_CURRENT_SOURCE_DIR}/dwf_spec.txt" >
                           "${CMAKE_CURRENT_BINARY_DIR}/dwf_spec.cc"
                   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mkdwfspec.py"
                           "${CMAKE_CURRENT_SOURCE_DIR}/dwf_spec.txt"
                           "${CMAKE_CURRENT_BINARY_DIR}/dwf_spec.h"
                  )

include_directories("${CMAKE_CURRENT_BINARY_DIR}"
                    .)

# libpathdb depends on libiberty
if(PATHDB_LIBIBERTY_PATH)
    link_directories("${PATHDB_LIBIBERTY_PATH}")
endif()

add_library(pathdb-lib STATIC ${pathdb_sources})
set_property(TARGET pathdb-lib PROPERTY OUTPUT_NAME "pathdb")
target_link_libraries(pathdb-lib dl ncurses iberty)

# Driver
add_executable(pathdb driver.cc)
target_link_libraries(pathdb pathdb-lib)

# doctool
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/doctext.cc"
                   COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/mkdoctext.py"
                           "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.xml" >
                           "${CMAKE_CURRENT_BINARY_DIR}/doctext.cc"
                   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mkdoctext.py"
                           "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.xml"
                  )
add_executable(doctool
               doctool.cc
               ${CMAKE_CURRENT_BINARY_DIR}/doctext.cc)

# funclookup
add_executable(funclookup funclookup.cc)
target_link_libraries(funclookup pathdb-lib)

# dependencies for stage
add_dependencies(pathdb-stage pathdb funclookup doctool)

# install
install(TARGETS pathdb funclookup doctool
        RUNTIME DESTINATION bin)

