project(core)
cmake_minimum_required(VERSION 2.6)

set(SRC
	Blip_Buffer/Blip_Buffer.cpp
	Blip_Buffer/Blip_Buffer.h
	
	
)

add_subdirectory("APU")

add_subdirectory("qt-gui")

add_library(core ${SRC})

