project(qt-gui)

set(SRC
	gui.cpp
	gui.hpp

	MainWindow.cpp
	PatternView.cpp
	PatternView.hpp
)

set(MOC_INC
	MainWindow.hpp
)

set(UI_FILES
	mainwindow.ui
)

find_package(Qt4 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})
setup_libs("${QT_LIBRARIES}")

# for the generated ui header files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(".")

QT4_WRAP_UI(UI_SRC ${UI_FILES})

QT4_WRAP_CPP(MOC_SRC ${MOC_INC})

add_library(qt-gui ${SRC} ${MOC_SRC} ${MOC_INC} ${UI_SRC})

