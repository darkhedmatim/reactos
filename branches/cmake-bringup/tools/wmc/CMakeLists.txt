
file(GLOB_RECURSE SOURCE "*.c")

add_executable(wmc ${SOURCE})

add_definitions(-DWINE_UNICODE_API= )

target_link_libraries(wmc unicode)
