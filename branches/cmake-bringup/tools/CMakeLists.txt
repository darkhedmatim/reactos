
add_library(xml xml.cpp)

#add_executable(pefixup pefixup.c)

add_subdirectory(buildno)
add_subdirectory(cabman)
add_subdirectory(cdmake)
add_subdirectory(gendib)
add_subdirectory(geninc)
add_subdirectory(mkhive)
add_subdirectory(nci)
add_subdirectory(spec2def)
add_subdirectory(unicode)

if(NOT MSVC)
add_subdirectory(wpp)
add_subdirectory(widl)
endif()
