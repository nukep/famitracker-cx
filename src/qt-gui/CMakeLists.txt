project(qt-gui)

set(SRC
	main.cpp

	gui.cpp
	gui.hpp

	MainWindow.cpp
	PatternView.cpp
	FrameView.cpp

	threads.cpp
	threads.hpp
)

set(MOC_INC
	MainWindow.hpp
	PatternView.hpp
	FrameView.hpp
)

set(UI_FILES
	mainwindow.ui
)

find_package(Qt4 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})
setup_libs("${QT_LIBRARIES}")

find_package(Boost REQUIRED thread)
include_directories(${Boost_INCLUDE_DIR})
setup_libs("${Boost_LIBRARIES}")

# for the generated ui header files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(".")

include_directories("..")

QT4_WRAP_UI(UI_SRC ${UI_FILES})

QT4_WRAP_CPP(MOC_SRC ${MOC_INC})

add_executable(famitracker-qt ${SRC} ${MOC_SRC} ${MOC_INC} ${UI_SRC})
target_link_libraries(famitracker-qt fami-core)
target_link_libraries(famitracker-qt ${QT_LIBRARIES} ${Boost_LIBRARIES})

install(TARGETS famitracker-qt
	RUNTIME DESTINATION bin
)
if (INSTALL_PORTABLE)
	install(PROGRAMS install/famitracker-qt.sh
		DESTINATION .
	)
endif()