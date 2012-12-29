project(ncurses-ui)

include_directories("..")

add_definitions(-DBOOST_ALL_NO_LIB)	# turn off boost auto-linking
find_package(Boost)
if (Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIR})
endif()

add_executable(famitracker-nc ../parse_arguments.cpp ../parse_arguments.hpp ../defaults.hpp main.cpp)
target_link_libraries(famitracker-nc fami-core -lncurses)

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

