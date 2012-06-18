include_directories("..")

find_package(ALSA REQUIRED)
include(${ALSA_INCLUDE_DIR})

add_library(core-alsa-sound MODULE alsa.cpp alsa.hpp)
target_link_libraries(core-alsa-sound ${ALSA_LIBRARY})

install(TARGETS core-alsa-sound
	LIBRARY DESTINATION lib
)

find_package(Jack)

if (JACK_FOUND)
	option(WITH_JACK "Use JACK audio" ON)
	if (WITH_JACK)
		include(${JACK_INCLUDE_DIR})
		add_library(core-jack-sound MODULE jack.cpp jack.hpp)
		target_link_libraries(core-jack-sound ${JACK_LIBRARY})

		install(TARGETS core-jack-sound
			LIBRARY DESTINATION lib
		)
	endif()
endif()
