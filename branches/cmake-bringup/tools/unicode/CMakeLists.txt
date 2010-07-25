
file(GLOB_RECURSE SOURCE "*.c")
string(REGEX REPLACE "fold.c" "" SOURCE "${SOURCE}")
string(REGEX REPLACE "sortkey.c" "" SOURCE "${SOURCE}")

add_definitions(-DWINE_UNICODE_API= )

add_library(unicode ${SOURCE})
