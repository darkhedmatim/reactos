list(APPEND SOURCE
    binhive.c
    cmi.c
    mkhive.c
    reginf.c
    registry.c
    rtl.c)

add_definitions(-DMKHIVE_HOST)
add_definitions(-DWINE_UNICODE_API= )

include_directories(${REACTOS_SOURCE_DIR}/lib/newinflib)
include_directories(${REACTOS_SOURCE_DIR}/lib/cmlib)
include_directories(${REACTOS_SOURCE_DIR}/lib/rtl)

add_executable(mkhive ${SOURCE})
target_link_libraries(mkhive unicode cmlibhost newinflibhost)