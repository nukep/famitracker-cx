#include <stdio.h>
#include "qt-gui/gui.hpp"

int main(int argc, char *argv[])
{
	printf("Welcome to Famitracker!\n");

	gui::init(argc, argv);

	gui::spin();

	gui::destroy();
	return 0;
}
