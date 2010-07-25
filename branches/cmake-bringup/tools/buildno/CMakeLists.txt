include_directories(..)

add_executable(buildno buildno.cpp)

target_link_libraries(buildno xml)

ADD_CUSTOM_COMMAND(OUTPUT ${REACTOS_SOURCE_DIR}/include/reactos/buildno.h
                   COMMAND buildno ${REACTOS_SOURCE_DIR}/include/reactos/buildno.h
                   DEPENDS ${REACTOS_SOURCE_DIR}/include/reactos/version.h ) 

ADD_CUSTOM_TARGET(build_header ALL DEPENDS ${REACTOS_SOURCE_DIR}/include/reactos/buildno.h)