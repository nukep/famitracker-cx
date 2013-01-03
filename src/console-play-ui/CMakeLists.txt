project(console-play-ui)

include_directories("..")

setup_boost()

add_executable(famitracker-play ../parse_arguments.cpp ../parse_arguments.hpp ../defaults.hpp main.cpp)
target_link_libraries(famitracker-play fami-core)

if (WIN32)
	install(TARGETS famitracker-play
		RUNTIME DESTINATION .
	)
else()
	install(TARGETS famitracker-play
		RUNTIME DESTINATION bin
	)
	if (INSTALL_PORTABLE)
		install(PROGRAMS install/famitracker-play.sh
			DESTINATION .
		)
	endif()
endif()
