project(console-play-ui)

include_directories("..")

add_definitions(-DBOOST_ALL_NO_LIB)	# turn off boost auto-linking
find_package(Boost)
if (Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIR})
endif()

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
