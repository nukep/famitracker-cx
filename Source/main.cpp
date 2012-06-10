#include <stdio.h>
#include "qt-gui/gui.hpp"

int main(int argc, char *argv[])
{
	printf("Welcome to FamiTracker!\n");

	gui::init(argc, argv);

	gui::spin();

	gui::destroy();
	return 0;
}
