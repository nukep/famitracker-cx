project(ncurses-ui)

include_directories("..")

setup_boost()

include_directories("${CURSES_INCLUDE_DIR}")

add_executable(famitracker-nc ../parse_arguments.cpp ../parse_arguments.hpp ../defaults.hpp main.cpp)
target_link_libraries(famitracker-nc fami-core "${CURSES_NCURSES_LIBRARY}" ${Boost_LIBRARIES})

if (WIN32)
	install(TARGETS famitracker-nc
		RUNTIME DESTINATION .
	)
else()
	install(TARGETS famitracker-nc
		RUNTIME DESTINATION bin
	)
	if (INSTALL_PORTABLE)
		install(PROGRAMS install/famitracker-nc.sh
			DESTINATION .
		)
	endif()
endif()

