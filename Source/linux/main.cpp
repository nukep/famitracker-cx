#include <stdio.h>
#include "../qt-gui/gui.hpp"
#include "Document.hpp"
#include "FtmDocument.hpp"

class FileIO : public IO
{
public:
	FileIO(const char *filename, const char *mode)
	{
		f = fopen(filename, mode);
	}

	Quantity read(void *buf, Quantity sz)
	{
		return fread(buf, 1, sz, f);
	}

	Quantity write(const void *buf, Quantity sz)
	{
		return fwrite(buf, 1, sz, f);
	}
	~FileIO()
	{
		fclose(f);
	}
	FILE *f;
};

int main(int argc, char *argv[])
{
	printf("Welcome to Famitracker!\n");

	FileIO f("./enl_track1.ftm", "rb");
	FtmDocument doc;
	doc.read(&f);

	gui::init(argc, argv);

	gui::spin();

	gui::destroy();
	return 0;
}
