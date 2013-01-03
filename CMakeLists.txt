# Created for Linux port by Dan Spencer

project(famitracker)
cmake_minimum_required(VERSION 2.6)

if ("${CMAKE_SYSTEM}" MATCHES "Linux")
	# turn off as-needed. cmake doesn't play nice with this, and it's on by default on ubuntu 11.10
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")
endif()

option(INSTALL_PORTABLE "Optimize installation for portable packaging" OFF)

option(UI_QT "Build the Qt GUI" ON)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
if (CURSES_FOUND)
	option(UI_NCURSES "Build the ncurses UI" ON)
endif()

set(CONFIG_H_DIR ${CMAKE_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CONFIG_H_DIR}/config.h)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules" CACHE STRING "")

add_subdirectory("src")
