project(core)
cmake_minimum_required(VERSION 2.6)

set(SRC
	common.hpp
	common/libraries.hpp
	common/platform.hpp

	io.cpp
	io.hpp

	module.cpp
	module.hpp
	types.hpp

	soundsink.cpp
	soundsink.hpp
	soundthread.cpp
	soundthread.hpp

	ringbuffer.hpp
	time.hpp
)

find_package(Boost REQUIRED thread)
if (Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIR})
endif()

add_library(core SHARED ${SRC})
target_link_libraries(core ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})

install(TARGETS core
	LIBRARY DESTINATION lib
)
