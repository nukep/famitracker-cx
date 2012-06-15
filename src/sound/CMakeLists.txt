include_directories("..")

find_package(ALSA REQUIRED)
include(${ALSA_INCLUDE_DIR})

add_library(fami-alsa-sound MODULE alsa.cpp alsa.hpp)
target_link_libraries(fami-alsa-sound ${ALSA_LIBRARY})

install(TARGETS fami-alsa-sound
	LIBRARY DESTINATION lib
)

find_package(Jack)

if (JACK_FOUND)
	option(WITH_JACK "Use JACK audio" ON)
	if (WITH_JACK)
		include(${JACK_INCLUDE_DIR})
		add_library(fami-jack-sound MODULE jack.cpp jack.hpp)
		target_link_libraries(fami-jack-sound ${JACK_LIBRARY})

		install(TARGETS fami-jack-sound
			LIBRARY DESTINATION lib
		)
	endif()
endif()
