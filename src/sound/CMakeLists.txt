include_directories("..")

add_definitions(-DBOOST_ALL_NO_LIB)	# turn off boost auto-linking
find_package(Boost)
if (Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIR})
endif()

if ("${CMAKE_SYSTEM}" MATCHES "Linux")
	find_package(ALSA REQUIRED)
	include(${ALSA_INCLUDE_DIR})

	add_library(core-alsa-sound MODULE alsa.cpp alsa.hpp)
	target_link_libraries(core-alsa-sound ${ALSA_LIBRARY})
	
	install(TARGETS core-alsa-sound
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

	add_library(core-dx-sound MODULE dsound.cpp dsound.hpp)
	target_link_libraries(core-dx-sound core "${DSOUND_LIBRARY}")

	install(TARGETS core-dx-sound
		DESTINATION .
	)
endif()

find_package(Jack)

if (JACK_FOUND)
	option(WITH_JACK "Use JACK audio" ON)
	if (WITH_JACK)
		include("${JACK_INCLUDE_DIR}")
		add_library(core-jack-sound MODULE jack.cpp jack.hpp)
		target_link_libraries(core-jack-sound "${JACK_LIBRARY}")

		install(TARGETS core-jack-sound
			LIBRARY DESTINATION lib
		)
	endif()
endif()
