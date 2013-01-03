project(core)
cmake_minimum_required(VERSION 2.6)

set(SRC
	common.hpp
	common/libraries.hpp
	common/platform.hpp

	io.cpp
	io.hpp

	types.hpp

	soundsink.cpp
	soundsink.hpp

	ringbuffer.hpp
	time.hpp
)

add_definitions(-DBOOST_ALL_NO_LIB)	# turn off boost auto-linking
find_package(Boost REQUIRED thread)
if (Boost_FOUND)
	if (NOT (${Boost_VERSION} LESS 105000))
		find_package(Boost REQUIRED system)
	endif()
	include_directories(${Boost_INCLUDE_DIR})
endif()

add_definitions(-DCORE_ISLIB)

add_library(famicx-common-core SHARED ${SRC})
target_link_libraries(famicx-common-core ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})

if (UNIX)
	install(TARGETS famicx-common-core
		LIBRARY DESTINATION lib
	)
else()
	install(TARGETS famicx-common-core
		DESTINATION .
	)
endif()

