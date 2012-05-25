# Created for Linux port by Dan Spencer
# Dependencies include Qt4 and ALSA

project(famitracker)
cmake_minimum_required(VERSION 2.6)

set(FT_LIBS "" CACHE INTERNAL "" FORCE)

macro(setup_libs LIBS)
	set(FT_LIBS ${FT_LIBS} ${LIBS} CACHE INTERNAL "" FORCE)
endmacro()

add_subdirectory("Source")

add_executable(famitracker Source/linux/main.cpp)
target_link_libraries(famitracker core apu qt-gui)
target_link_libraries(famitracker ${FT_LIBS})

