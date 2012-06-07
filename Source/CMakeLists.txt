project(core)
cmake_minimum_required(VERSION 2.6)

set(SRC
	common.hpp
	exceptions.cpp
	exceptions.hpp

	alsa.cpp
	alsa.hpp
	wavoutput.cpp
	wavoutput.hpp
	sound.cpp
	sound.hpp

	App.cpp
	App.hpp

	ChannelHandler.cpp
	ChannelHandler.h
	Channels2A03.cpp
	Channels2A03.h
	ChannelsMMC5.cpp
	ChannelsMMC5.h
	ChannelsVRC6.cpp
	ChannelsVRC6.h
	ChannelMap.cpp
	ChannelMap.h

	Document.cpp
	Document.hpp
	FtmDocument.cpp
	FtmDocument.hpp
	CustomExporterInterfaces.h
	FamiTrackerTypes.h
	Instrument.cpp
	Instrument.h
	Instrument2A03.cpp
	InstrumentVRC6.cpp
	PatternData.cpp
	PatternData.h
	Sequence.cpp
	Sequence.h
	TrackerChannel.cpp
	TrackerChannel.h
	TrackerController.cpp
	TrackerController.hpp
	types.hpp

	Settings.cpp
	Settings.h
)

add_subdirectory("APU")

find_package(ALSA REQUIRED)
include(${ALSA_INCLUDE_DIR})
setup_libs("${ALSA_LIBRARY}")

add_library(core ${SRC})
target_link_libraries(core apu)
