project(core)
cmake_minimum_required(VERSION 2.6)

set(SRC
	linux/alsa.cpp
	linux/alsa.hpp
	linux/sound.hpp
	linux/App.cpp
	linux/App.hpp
	linux/Document.cpp
	linux/Document.hpp
	linux/FtmDocument.cpp
	linux/FtmDocument.hpp
	linux/CustomExporterInterfaces.h
	linux/FamiTrackerTypes.h
	linux/Instrument.cpp
	linux/Instrument.h
	linux/Instrument2A03.cpp
	linux/PatternData.cpp
	linux/PatternData.h
	linux/Sequence.cpp
	linux/Sequence.h
	linux/TrackerChannel.cpp
	linux/TrackerChannel.h
	linux/types.hpp
)

add_subdirectory("APU")

add_subdirectory("qt-gui")

add_library(core ${SRC})

