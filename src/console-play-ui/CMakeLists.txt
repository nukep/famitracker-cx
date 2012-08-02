project(console-play-ui)

include_directories("..")

add_executable(famitracker-play ../parse_arguments.cpp ../parse_arguments.hpp main.cpp)
target_link_libraries(famitracker-play fami-core)

install(TARGETS famitracker-play
	RUNTIME DESTINATION bin
)
if (INSTALL_PORTABLE)
	install(PROGRAMS install/famitracker-play.sh
		DESTINATION .
	)
endif()
