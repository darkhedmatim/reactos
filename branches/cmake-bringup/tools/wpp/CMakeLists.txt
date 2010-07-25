
file(GLOB_RECURSE SOURCE "*.c")

SET_SOURCE_FILES_PROPERTIES(${REACTOS_SOURCE_DIR}/include/reactos/buildno.h PROPERTIES GENERATED 1)

add_library(wpp ${SOURCE})

ADD_DEPENDENCIES(wpp build_header)