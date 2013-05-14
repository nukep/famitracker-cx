project(qt-gui)

set(SRC
	../parse_arguments.cpp
	../parse_arguments.hpp
	../defaults.hpp
	../version.hpp

	main.cpp

	GUI.cpp
	GUI.hpp
	GUI_App.cpp
	GUI_App.hpp
	GUI_Sound.cpp
	GUI_Sound.cpp
	GUI_ThreadPool.cpp
	GUI_ThreadPool.hpp
	DocInfo.cpp
	DocInfo.hpp

	styles.cpp
	styles.hpp

	MainWindow.cpp
	CreateWAV.cpp
	ModuleProperties.cpp
	PatternView.cpp
	FrameView.cpp
	AboutDialog.cpp
	
	RowPages.cpp
	RowPages.hpp

	SequenceEditor.cpp

	InstrumentEditor.cpp
	InstrumentEditor.hpp
	InstrumentEditorTypes.cpp

	Settings.cpp
	Settings.hpp
)

if (WIN32)
	list(APPEND SRC FamiTracker.rc)
endif()

set(MOC_INC
	MainWindow.hpp
	CreateWAV.hpp
	ModuleProperties.hpp
	PatternView.hpp
	FrameView.hpp
	AboutDialog.hpp

	SequenceEditor.hpp
	InstrumentEditorTypes.hpp
)

set(UI_FILES
	mainwindow.ui
	createwav.ui
	moduleproperties.ui
	about.ui
)

find_package(Qt4 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})

setup_boost()

# for the generated ui header files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(".")

include_directories("..")

# for config.h
include_directories("${CONFIG_H_DIR}")

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
