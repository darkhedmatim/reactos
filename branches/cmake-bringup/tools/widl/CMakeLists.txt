
file(GLOB_RECURSE SOURCE "*.c")

# Taken from widl.rbuild
add_definitions(-DINT16=SHORT)

add_executable(widl ${SOURCE})

target_link_libraries(widl wpp)