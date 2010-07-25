
file(GLOB_RECURSE SOURCE "*.c")

add_executable(winebuild ${SOURCE} ${REACTOS_SOURCE_DIR}/include/reactos/buildno.h)

ADD_DEPENDENCIES(winebuild build_header)