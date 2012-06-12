# Created for Linux port by Dan Spencer
# Dependencies include Qt4 and ALSA

project(famitracker)
cmake_minimum_required(VERSION 2.6)

set(FT_LIBS "" CACHE INTERNAL "" FORCE)

macro(setup_libs LIBS)
	set(FT_LIBS ${FT_LIBS} ${LIBS} CACHE INTERNAL "" FORCE)
endmacro()

if ("${CMAKE_SYSTEM}" MATCHES "Linux")
	# turn off as-needed. cmake doesn't play nice with this, and it's on by default on ubuntu 11.10
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")
endif()

option(INSTALL_PORTABLE "Optimize installation for portable packaging" OFF)

option(UI_QT "Build the Qt GUI" ON)

add_subdirectory("src")
