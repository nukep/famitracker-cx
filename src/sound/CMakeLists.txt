include_directories("..")

setup_boost()

set(SOUNDTHREAD
	soundthread.cpp
	soundthread.hpp)

if ("${CMAKE_SYSTEM}" MATCHES "Linux")
	find_package(ALSA REQUIRED)
	include(${ALSA_INCLUDE_DIR})

	add_library(famicx-core-alsa-sound MODULE alsa.cpp alsa.hpp ${SOUNDTHREAD})
	target_link_libraries(famicx-core-alsa-sound ${ALSA_LIBRARY})
	
	install(TARGETS famicx-core-alsa-sound
		LIBRARY DESTINATION lib
	)
endif()
if (WIN32)
	# directsound

	find_path(DSOUND_INCLUDE_DIR
		NAMES dsound.h
	)

	find_library(DSOUND_LIBRARY
		NAMES dsound
	)

	include_directories("${DSOUND_INCLUDE_DIR}")

	add_library(famicx-core-dx-sound MODULE dsound.cpp dsound.hpp)
	target_link_libraries(famicx-core-dx-sound core "${DSOUND_LIBRARY}")

	install(TARGETS famicx-core-dx-sound
		DESTINATION .
	)
endif()

find_package(Jack)

if (JACK_FOUND)
	option(WITH_JACK "Use JACK audio" ON)
	if (WITH_JACK)
		include("${JACK_INCLUDE_DIR}")
		add_library(famicx-core-jack-sound MODULE jack.cpp jack.hpp)
		target_link_libraries(famicx-core-jack-sound "${JACK_LIBRARY}")

		install(TARGETS famicx-core-jack-sound
			LIBRARY DESTINATION lib
		)
	endif()
endif()
