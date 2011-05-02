
list(APPEND SOURCE
    lex.yy.c
    preproc.c
    wpp.c
    ppy.tab.c)

set_source_files_properties(${REACTOS_BINARY_DIR}/include/reactos/buildno.h PROPERTIES GENERATED TRUE)

add_library(wpp ${SOURCE})

add_dependencies(wpp build_header)