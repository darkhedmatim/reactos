
add_definitions(-DEXEEXT=".exe")

file(GLOB_RECURSE SOURCE "*.c")

add_executable(winebuild ${SOURCE} ${REACTOS_BINARY_DIR}/include/reactos/buildno.h)

SET_SOURCE_FILES_PROPERTIES(${REACTOS_BINARY_DIR}/include/reactos/buildno.h PROPERTIES GENERATED TRUE)

ADD_DEPENDENCIES(winebuild build_header)