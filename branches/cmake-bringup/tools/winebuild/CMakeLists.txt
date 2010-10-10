
list(APPEND SOURCE
    import.c
    main.c
    mkstemps.c
    parser.c
    relay.c
    res16.c
    res32.c
    spec16.c
    spec32.c
    utils.c
)

add_definitions(-DEXEEXT=".exe")
if(NOT UNIX)
add_definitions(-Dpclose=_pclose)
add_definitions(-Dpopen=_popen)
endif()
if(MSVC)
    list(APPEND SOURCE getopt.c getpid.c)
    include_directories(include)
endif()

add_executable(winebuild ${SOURCE} ${REACTOS_BINARY_DIR}/include/reactos/buildno.h)

SET_SOURCE_FILES_PROPERTIES(${REACTOS_BINARY_DIR}/include/reactos/buildno.h PROPERTIES GENERATED TRUE)

ADD_DEPENDENCIES(winebuild build_header)
