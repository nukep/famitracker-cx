project(core)
cmake_minimum_required(VERSION 2.6)

set(SRC
	linux/common.hpp
	linux/exceptions.cpp
	linux/exceptions.hpp

	linux/alsa.cpp
	linux/alsa.hpp
	linux/wavoutput.cpp
	linux/wavoutput.hpp
	linux/sound.cpp
	linux/sound.hpp

	linux/App.cpp
	linux/App.hpp

	linux/ChannelHandler.cpp
	linux/ChannelHandler.h
	linux/Channels2A03.cpp
	linux/Channels2A03.h
	linux/ChannelsVRC6.cpp
	linux/ChannelsVRC6.h
	linux/ChannelMap.cpp
	linux/ChannelMap.h

	linux/Document.cpp
	linux/Document.hpp
	linux/FtmDocument.cpp
	linux/FtmDocument.hpp
	linux/CustomExporterInterfaces.h
	linux/FamiTrackerTypes.h
	linux/Instrument.cpp
	linux/Instrument.h
	linux/Instrument2A03.cpp
	linux/InstrumentVRC6.cpp
	linux/PatternData.cpp
	linux/PatternData.h
	linux/Sequence.cpp
	linux/Sequence.h
	linux/TrackerChannel.cpp
	linux/TrackerChannel.h
	linux/TrackerController.cpp
	linux/TrackerController.hpp
	linux/types.hpp

	linux/Settings.cpp
	linux/Settings.h
)

add_subdirectory("APU")

add_subdirectory("qt-gui")

find_package(ALSA REQUIRED)
include(${ALSA_INCLUDE_DIR})
setup_libs("${ALSA_LIBRARY}")

add_library(core ${SRC})
target_link_libraries(core apu)
