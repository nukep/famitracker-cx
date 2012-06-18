add_subdirectory("core")
add_subdirectory("famitracker-core")
add_subdirectory("sound")

add_subdirectory("console-play-ui")

if (UI_QT)
	add_subdirectory("qt-gui")
endif()

