
add_definitions(-DEXEEXT=".exe")

file(GLOB_RECURSE SOURCE "*.c")

add_executable(winebuild ${SOURCE} ${REACTOS_SOURCE_DIR}/include/reactos/buildno.h)

SET_SOURCE_FILES_PROPERTIES(${REACTOS_SOURCE_DIR}/include/reactos/buildno.h PROPERTIES GENERATED 1)

ADD_DEPENDENCIES(winebuild build_header)