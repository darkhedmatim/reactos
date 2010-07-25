
file(GLOB_RECURSE SOURCE "*.c")

LIST(REMOVE_ITEM SOURCE "fold.c")
LIST(REMOVE_ITEM SOURCE "sortkey.c")

add_definitions(-DWINE_UNICODE_API= )

add_library(unicode ${SOURCE})
