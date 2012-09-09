project(qt-gui)

set(SRC
	../parse_arguments.cpp
	../parse_arguments.hpp
	../defaults.hpp
	../version.hpp

	main.cpp

	gui.cpp
	gui.hpp

	styles.cpp
	styles.hpp

	MainWindow.cpp
	PatternView.cpp
	FrameView.cpp

	SequenceEditor.cpp

	InstrumentEditor.cpp
	InstrumentEditor.hpp
	InstrumentEditorTypes.cpp
)

if (WIN32)
	list(APPEND SRC FamiTracker.rc)
endif()

set(MOC_INC
	MainWindow.hpp
	PatternView.hpp
	FrameView.hpp

	SequenceEditor.hpp
	InstrumentEditorTypes.hpp
)

set(UI_FILES
	mainwindow.ui
)

find_package(Qt4 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})
setup_libs("${QT_LIBRARIES}")

add_definitions(-DBOOST_ALL_NO_LIB)	# turn off boost auto-linking
find_package(Boost REQUIRED thread)
include_directories(${Boost_INCLUDE_DIR})
setup_libs("${Boost_LIBRARIES}")

# for the generated ui header files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(".")

include_directories("..")

QT4_ADD_RESOURCES(RES_SRC resources.qrc)

QT4_WRAP_UI(UI_SRC ${UI_FILES})

QT4_WRAP_CPP(MOC_SRC ${MOC_INC})

add_executable(famitracker-qt WIN32 ${SRC} ${RES_SRC} ${MOC_SRC} ${MOC_INC} ${UI_SRC})
target_link_libraries(famitracker-qt fami-core)
target_link_libraries(famitracker-qt ${QT_LIBRARIES} ${Boost_LIBRARIES})

if (WIN32)
	install(TARGETS famitracker-qt
		RUNTIME DESTINATION .
	)
else()
	install(TARGETS famitracker-qt
		RUNTIME DESTINATION bin
	)
	if (INSTALL_PORTABLE)
		install(PROGRAMS install/famitracker-qt.sh
			DESTINATION .
		)
	endif()
endif()
