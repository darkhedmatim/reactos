
file(GLOB_RECURSE SOURCE "*.c")

add_executable(wrc ${SOURCE})

add_definitions(-DWINE_UNICODE_API= )

target_link_libraries(wrc wpp unicode)
