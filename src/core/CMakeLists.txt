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

	threadpool.cpp
	threadpool.hpp
)

setup_boost()

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

