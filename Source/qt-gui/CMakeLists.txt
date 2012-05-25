project(qt-gui)

set(SRC
	gui.cpp
)

set(MOC_INC
)

find_package(Qt4 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})
setup_libs("${QT_LIBRARIES}")

QT4_WRAP_CPP(MOC_SRC ${MOC_INC})

add_library(qt-gui ${SRC} ${MOC_SRC} ${MOC_INC})

