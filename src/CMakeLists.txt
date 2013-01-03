macro(setup_boost)
	add_definitions(-DBOOST_ALL_NO_LIB)	# turn off boost auto-linking
	find_package(Boost REQUIRED thread)
	if (Boost_FOUND)
		if (NOT (${Boost_VERSION} LESS 105000))
			# boost 1.50+ requires system
			find_package(Boost REQUIRED system)
		endif()
		include_directories(${Boost_INCLUDE_DIR})
	endif()
endmacro()

add_subdirectory("core")
add_subdirectory("famitracker-core")
add_subdirectory("sound")

add_subdirectory("console-play-ui")

if (UI_NCURSES)
	add_subdirectory("ncurses-ui")
endif()

if (UI_QT)
	add_subdirectory("qt-gui")
endif()

