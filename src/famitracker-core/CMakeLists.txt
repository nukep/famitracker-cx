project(famitracker-core)
cmake_minimum_required(VERSION 2.6)

include_directories("..")

set(SRC
	common.hpp
	exceptions.cpp
	exceptions.hpp

#	wavoutput.cpp
#	wavoutput.hpp
	sound.cpp
	sound.hpp

	App.cpp
	App.hpp

	ChannelHandler.cpp
	ChannelHandler.h
	Channels2A03.cpp
	Channels2A03.h
	ChannelsFDS.cpp
	ChannelsFDS.h
	ChannelsMMC5.cpp
	ChannelsMMC5.h
	ChannelsVRC6.cpp
	ChannelsVRC6.h
	ChannelsVRC7.cpp
	ChannelsVRC7.h
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
	InstrumentFDS.cpp
	InstrumentVRC6.cpp
	InstrumentVRC7.cpp
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

set(CUR_SRC "${CMAKE_CURRENT_SOURCE_DIR}")

macro (add_sources)
	file (RELATIVE_PATH _relPath "${CUR_SRC}" "${CMAKE_CURRENT_SOURCE_DIR}")
	foreach (_src ${ARGN})
		if (_relPath)
			list (APPEND SRC "${_relPath}/${_src}")
		else()
			list (APPEND SRC "${_src}")
		endif()
	endforeach()
	if (_relPath)
		# propagate SRCS to parent directory
		set (SRC ${SRC} PARENT_SCOPE)
	endif()
endmacro()

add_subdirectory("APU")

add_definitions(-DBOOST_ALL_NO_LIB)	# turn off boost auto-linking
find_package(Boost REQUIRED thread)
include_directories(${Boost_INCLUDE_DIR})

add_definitions(-DFAMICORE_ISLIB)

add_library(fami-core SHARED ${SRC})
target_link_libraries(fami-core core ${Boost_LIBRARIES})

if (UNIX)
	install(TARGETS fami-core
		LIBRARY DESTINATION lib
	)
else()
	install(TARGETS fami-core
		DESTINATION .
	)
endif()
